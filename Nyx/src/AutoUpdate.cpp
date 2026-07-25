#include "AutoUpdate.hpp"

#include <winhttp.h>
#include <wininet.h>
#include <urlmon.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")

namespace AutoUpdate
{

void LoadUrlOverrides()
{
    const auto path = ThisExePath().parent_path() / "update_urls.txt";
    std::ifstream in(path);
    if (!in) return;
    std::string line;
    while (std::getline(in, line))
    {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = Trim(line.substr(0, eq));
        std::string v = Trim(line.substr(eq + 1));
        for (auto& c : k) c = (char)std::tolower((unsigned char)c);
        if (k == "version" || k == "version_url") g_versionUrl = v;
        else if (k == "exe" || k == "exe_url") g_exeUrl = v;
    }
}

static bool RunHidden(const std::wstring& cmd, DWORD timeoutMs = 60000)
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(0);
    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        return false;
    DWORD wait = WaitForSingleObject(pi.hProcess, timeoutMs);
    DWORD code = 1;
    if (wait == WAIT_OBJECT_0)
        GetExitCodeProcess(pi.hProcess, &code);
    else
        TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

static bool DownloadBytesCurl(const std::string& url, std::vector<char>& out)
{
    out.clear();
    auto tmp = MakeTempFile("ric");
    std::wstring cmd = L"curl.exe -fsSL --connect-timeout 20 --max-time 90 "
        L"-A \"Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/122.0.0.0\" "
        L"-o \"" + tmp.wstring() + L"\" \"" + ToWide(url) + L"\"";
    if (!RunHidden(cmd, 100000))
    {
        std::error_code ec; std::filesystem::remove(tmp, ec);
        return false;
    }
    bool ok = ReadFileBytes(tmp, out);
    std::error_code ec; std::filesystem::remove(tmp, ec);
    return ok;
}

static bool DownloadBytesPowerShell(const std::string& url, std::vector<char>& out)
{
    out.clear();
    auto tmp = MakeTempFile("rip");
    std::wstring cmd =
        L"powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command "
        L"\"try { Invoke-WebRequest -Uri '" + ToWide(url) + L"' -OutFile '" +
        tmp.wstring() + L"' -UseBasicParsing -TimeoutSec 60; exit 0 } catch { exit 1 }\"";
    if (!RunHidden(cmd, 90000))
    {
        std::error_code ec; std::filesystem::remove(tmp, ec);
        return false;
    }
    bool ok = ReadFileBytes(tmp, out);
    std::error_code ec; std::filesystem::remove(tmp, ec);
    return ok;
}

static bool DownloadBytesWinHttp(const std::string& url, std::vector<char>& out)
{
    out.clear();
    std::wstring wurl = ToWide(url);

    URL_COMPONENTSW uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256]{};
    wchar_t path[2048]{};
    wchar_t extra[1024]{};
    uc.lpszHostName = host; uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path; uc.dwUrlPathLength = 2048;
    uc.lpszExtraInfo = extra; uc.dwExtraInfoLength = 1024;

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc))
        return false;

    HINTERNET hSession = WinHttpOpen(
        ToWide(kBrowserUA).c_str(),
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession)
    {
        hSession = WinHttpOpen(
            ToWide(kBrowserUA).c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
    }
    if (!hSession) return false;

    DWORD timeout = 45000;
    WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);

    DWORD redir = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &redir, sizeof(redir));

    INTERNET_PORT port = uc.nPort ? uc.nPort :
        ((uc.nScheme == INTERNET_SCHEME_HTTPS) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT);

    HINTERNET hConnect = WinHttpConnect(hSession, host, port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    std::wstring fullPath = path;
    if (extra[0]) fullPath += extra;

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET", fullPath.c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (flags & WINHTTP_FLAG_SECURE)
    {
        DWORD sec =
            SECURITY_FLAG_IGNORE_UNKNOWN_CA |
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
            SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &sec, sizeof(sec));
    }

    const wchar_t* hdrs =
        L"Accept: text/plain,application/octet-stream,*/*\r\n"
        L"Cache-Control: no-cache\r\n";

    BOOL ok = WinHttpSendRequest(hRequest, hdrs, (DWORD)-1L,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (ok) ok = WinHttpReceiveResponse(hRequest, nullptr);

    if (ok)
    {
        DWORD status = 0, sz = sizeof(status);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);

        if (status >= 200 && status < 400)
        {
            for (;;)
            {
                DWORD avail = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &avail) || avail == 0)
                    break;
                size_t old = out.size();
                out.resize(old + avail);
                DWORD read = 0;
                if (!WinHttpReadData(hRequest, out.data() + old, avail, &read) || read == 0)
                {
                    out.resize(old);
                    break;
                }
                out.resize(old + read);
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return !out.empty();
}

static bool DownloadBytesWinINet(const std::string& url, std::vector<char>& out)
{
    out.clear();
    HINTERNET hInternet = InternetOpenA(kBrowserUA, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    DWORD timeout = 45000;
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    const DWORD flags =
        INTERNET_FLAG_RELOAD |
        INTERNET_FLAG_NO_CACHE_WRITE |
        INTERNET_FLAG_NO_UI |
        INTERNET_FLAG_KEEP_CONNECTION |
        INTERNET_FLAG_PRAGMA_NOCACHE |
        INTERNET_FLAG_SECURE;

    const char* headers =
        "Accept: text/plain,application/octet-stream,*/*\r\n"
        "Cache-Control: no-cache\r\n";

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), headers, (DWORD)-1L, flags, 0);
    if (!hUrl)
        hUrl = InternetOpenUrlA(hInternet, url.c_str(), headers, (DWORD)-1L,
            flags & ~INTERNET_FLAG_SECURE, 0);
    if (!hUrl)
    {
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[65536];
    DWORD bytesRead = 0;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
        out.insert(out.end(), buffer, buffer + bytesRead);

    if (!out.empty() && LooksLikeHtml(std::string(out.begin(), out.begin() + (std::min)(out.size(), size_t(400)))))
    {
        InternetCloseHandle(hUrl);
        hUrl = InternetOpenUrlA(hInternet, url.c_str(), headers, (DWORD)-1L, flags, 0);
        if (hUrl)
        {
            std::vector<char> second;
            while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
                second.insert(second.end(), buffer, buffer + bytesRead);
            if (!second.empty() && !LooksLikeHtml(std::string(second.begin(),
                    second.begin() + (std::min)(second.size(), size_t(400)))))
                out.swap(second);
        }
    }

    if (hUrl) InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return !out.empty();
}

static bool DownloadBytesUrlMon(const std::string& url, std::vector<char>& out)
{
    out.clear();
    char tempPath[MAX_PATH]{};
    char tempFile[MAX_PATH]{};
    if (!GetTempPathA(MAX_PATH, tempPath)) return false;
    if (!GetTempFileNameA(tempPath, "riu", 0, tempFile)) return false;

    HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), tempFile, 0, nullptr);
    if (FAILED(hr))
    {
        DeleteFileA(tempFile);
        return false;
    }

    std::ifstream in(tempFile, std::ios::binary);
    if (!in)
    {
        DeleteFileA(tempFile);
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    in.close();
    DeleteFileA(tempFile);
    return !out.empty();
}

static bool DownloadBytes(const std::string& url, std::vector<char>& out, std::string* err = nullptr)
{
    out.clear();
    if (DownloadBytesCurl(url, out) && !out.empty() && !LooksLikeHtml(std::string(out.data(), (std::min)(out.size(), size_t(200)))))
        return true;
    out.clear();
    if (DownloadBytesPowerShell(url, out) && !out.empty() && !LooksLikeHtml(std::string(out.data(), (std::min)(out.size(), size_t(200)))))
        return true;
    out.clear();
    if (DownloadBytesWinHttp(url, out) && !out.empty() && !LooksLikeHtml(std::string(out.data(), (std::min)(out.size(), size_t(200)))))
        return true;
    out.clear();
    if (DownloadBytesWinINet(url, out) && !out.empty() && !LooksLikeHtml(std::string(out.data(), (std::min)(out.size(), size_t(200)))))
        return true;
    out.clear();
    if (DownloadBytesUrlMon(url, out) && !out.empty() && !LooksLikeHtml(std::string(out.data(), (std::min)(out.size(), size_t(200)))))
        return true;
    out.clear();
    if (err) *err = "all download backends failed";
    return false;
}

static std::string DownloadString(const std::string& url)
{
    std::vector<char> bytes;
    if (!DownloadBytes(url, bytes)) return {};
    std::string s = Trim(std::string(bytes.begin(), bytes.end()));
    if (LooksLikeHtml(s)) return {};
    if (s.size() < 3) return {};
    return s;
}

bool DownloadFile(const std::string& url, const std::filesystem::path& dest)
{
    std::vector<char> bytes;
    if (!DownloadBytes(url, bytes) || bytes.size() < 64)
        return false;
    if (LooksLikeHtml(std::string(bytes.begin(), bytes.begin() + (std::min)(bytes.size(), size_t(200)))))
        return false;
    std::ofstream out(dest, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    out.close();
    return true;
}

static int ForceKillOtherInstances()
{
    const DWORD self = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    int killed = 0;
    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (pe.th32ProcessID == self) continue;
            std::wstring name = pe.szExeFile;
            for (auto& c : name) c = (wchar_t)towlower(c);
            if (name != L"robloxini.exe") continue;
            HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
            if (h)
            {
                if (TerminateProcess(h, 0)) ++killed;
                CloseHandle(h);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return killed;
}

static bool ParseManifest(const std::string& body, std::string& outVersion, std::string& outExeUrl)
{
    outVersion.clear();
    outExeUrl.clear();
    if (body.empty()) return false;
    if (LooksLikeHtml(body)) return false;

    std::string clean = body;
    auto takeToken = [](const std::string& s, size_t from, size_t* next) -> std::string {
        size_t i = from;
        while (i < s.size() && (unsigned char)s[i] <= ' ') ++i;
        size_t j = i;
        while (j < s.size() && (unsigned char)s[j] > ' ') ++j;
        if (next) *next = j;
        return s.substr(i, j - i);
    };

    for (char& c : clean)
        if (c == '\r' || c == '\n' || c == ';' || c == ',') c = ' ';

    size_t pos = 0;
    while (pos < clean.size())
    {
        std::string tok = takeToken(clean, pos, &pos);
        if (tok.empty()) break;
        if (tok[0] == '#') continue;
        if (tok.rfind("version=", 0) == 0)
        {
            outVersion = Trim(tok.substr(8));
            continue;
        }
        if (tok.rfind("exe=", 0) == 0)
        {
            outExeUrl = Trim(tok.substr(4));
            continue;
        }
        if (tok.rfind("enabled=", 0) == 0)
            continue;
        if (LooksLikeUrl(tok))
        {
            if (outExeUrl.empty()) outExeUrl = tok;
            continue;
        }
        if (outVersion.empty() && tok.find_first_not_of("0123456789.") == std::string::npos)
        {
            int dots = 0;
            for (char c : tok) if (c == '.') ++dots;
            if (dots <= 2)
                outVersion = tok;
        }
    }
    return !outVersion.empty();
}

static std::string UnwrapGitHubContentsJson(std::string body)
{
    if (body.find("enabled=") != std::string::npos && body.find("\"content\"") == std::string::npos)
        return body;

    const char* key = "\"content\"";
    auto pos = body.find(key);
    if (pos == std::string::npos) return body;
    pos = body.find(':', pos);
    if (pos == std::string::npos) return body;
    pos = body.find('"', pos + 1);
    if (pos == std::string::npos) return body;
    ++pos;
    auto end = body.find('"', pos);
    if (end == std::string::npos) return body;
    std::string b64 = body.substr(pos, end - pos);

    std::string clean;
    clean.reserve(b64.size());
    for (size_t i = 0; i < b64.size(); ++i)
    {
        char c = b64[i];
        if (c == '\\' && i + 1 < b64.size())
        {
            char nx = b64[i + 1];
            if (nx == 'n' || nx == 'r' || nx == 't') { ++i; continue; }
            if (nx == '\\') { clean.push_back('\\'); ++i; continue; }
            if (nx == '"')  { clean.push_back('"');  ++i; continue; }
            ++i;
            if (i < b64.size() && b64[i] != '\n' && b64[i] != '\r' && b64[i] != ' ')
                clean.push_back(b64[i]);
            continue;
        }
        if (c != '\n' && c != '\r' && c != ' ')
            clean.push_back(c);
    }

    auto val = [](unsigned char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    std::string out;
    int valb = -8;
    int buffer = 0;
    for (unsigned char c : clean)
    {
        if (c == '=') break;
        int d = val(c);
        if (d < 0) continue;
        buffer = (buffer << 6) | d;
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(static_cast<char>((buffer >> valb) & 0xFF));
            valb -= 8;
        }
    }
    if (!out.empty()) return out;
    return body;
}

static int StatusVote(std::string body)
{
    if (body.empty()) return -1;
    body = UnwrapGitHubContentsJson(std::move(body));
    for (auto& c : body) c = (char)std::tolower((unsigned char)c);

    if (body.find("enabled=0") != std::string::npos
        || body.find("enabled=false") != std::string::npos
        || body.find("enabled=no") != std::string::npos
        || body.find("kill=1") != std::string::npos
        || body.find("killswitch=1") != std::string::npos
        || body.find("disabled=1") != std::string::npos)
        return 1;

    const std::string t = Trim(body);
    if (t == "0" || t == "off" || t == "kill" || t == "disabled" || t == "disable")
        return 1;

    if (body.find("disabled") != std::string::npos && body.find("enabled=1") == std::string::npos)
        return 1;

    if (body.find("enabled=1") != std::string::npos
        || body.find("enabled=true") != std::string::npos
        || body.find("enabled=yes") != std::string::npos)
        return 0;

    return -1;
}

static std::string DownloadVersionManifest()
{
    std::vector<std::string> urls;
    urls.push_back(g_versionUrl);
    urls.emplace_back(kDefaultVersionUrl);
    for (const char* u : kVersionFallbacks)
        if (u && *u) urls.emplace_back(u);

    std::string bestBody;
    std::string bestVer;
    int bestInt = -1;
    std::vector<std::string> tried;

    for (const auto& base : urls)
    {
        bool seen = false;
        for (const auto& t : tried) if (t == base) { seen = true; break; }
        if (seen) continue;
        tried.push_back(base);

        const std::string variants[2] = { WithCacheBust(base), base };
        for (int vi = 0; vi < 2; ++vi)
        {
            const std::string& u = variants[vi];
            std::string body = DownloadString(u);
            if (body.empty()) continue;

            if (body.find("\"content\"") != std::string::npos)
                body = UnwrapGitHubContentsJson(body);

            if (body.find("\"content\"") != std::string::npos || body.find("\"sha\"") != std::string::npos)
                continue;

            std::string ver, exe;
            if (!ParseManifest(body, ver, exe) || ver.empty())
                continue;
            const int vNum = VersionToInt(ver);

            bool preferThis = (vNum > bestInt) ||
                (vNum == bestInt && !bestBody.empty() &&
                 bestBody.find("\"content\"") != std::string::npos &&
                 body.find("\"content\"") == std::string::npos);
            if (preferThis)
            {
                bestInt = vNum;
                bestVer = ver;
                bestBody = body;
            }
            break;
        }
    }

    if (bestBody.empty())
        std::cout << "[!] no version host reachable\n";
    return bestBody;
}

static void WriteUpdateUrlsFile(const std::filesystem::path& dir)
{
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    const auto path = dir / "update_urls.txt";
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;
    out << "# robloxini update channel\n";
    out << "version=https://api.github.com/repos/bellly1/Nyx-external/contents/version.txt\n";
    out << "exe=https://raw.githubusercontent.com/bellly1/Nyx-external/main/Nyx.bin\n";
}

bool CheckRemoteDisable(bool quiet)
{
    auto tryUrl = [&](const std::string& url) -> int {
        std::string body = DownloadString(WithCacheBust(url));
        if (body.empty())
            body = DownloadString(url);
        if (body.empty())
            return -1;
        return StatusVote(body);
    };

    {
        const int v = tryUrl(kDefaultStatusUrl);
        if (v == 1)
        {
            if (!quiet)
            {
                std::cout << "\n[!] DISABLED via GitHub status.txt (enabled=0)\n";
                std::cout << "[!] To re-enable: set status.txt to enabled=1 and save.\n\n";
            }
            return true;
        }
        if (v == 0)
            return false;
    }

    for (const char* u : kStatusFallbacks)
    {
        if (!u || !*u) continue;
        if (std::string(u) == kDefaultStatusUrl) continue;
        const int v = tryUrl(u);
        if (v == 1)
        {
            if (!quiet)
            {
                std::cout << "\n[!] DISABLED via GitHub status.txt\n";
                std::cout << "[!] To re-enable: set status.txt to enabled=1 and save.\n\n";
            }
            return true;
        }
        if (v == 0)
            return false;
    }

    return false;
}

bool CheckAndUpdate()
{
    LoadUrlOverrides();

    if (CheckRemoteDisable(false))
        return true;

    std::cout << "[*] checking for updates (local v" << kAppVersion << ")...\n";

    std::string remoteBody = DownloadVersionManifest();
    if (remoteBody.empty())
    {
        std::cout << "[*] no update server reachable (ok - continuing)\n";
        return false;
    }

    std::string remoteVer, remoteExe;
    if (!ParseManifest(remoteBody, remoteVer, remoteExe))
    {
        std::cout << "[*] update data unreadable (ok - continuing)\n";
        return false;
    }

    if (remoteExe.empty())
        remoteExe = g_exeUrl;

    if (remoteExe.empty() || !LooksLikeUrl(remoteExe))
    {
        std::string vurl = g_versionUrl;
        const char* tags[] = { "version.txt", "version" };
        for (const char* t : tags)
        {
            auto pos = vurl.rfind(t);
            if (pos != std::string::npos)
            {
                remoteExe = vurl.substr(0, pos) + "robloxini.bin";
                break;
            }
        }
    }

    if (!RemoteIsNewer(remoteVer, kAppVersion))
    {
        std::cout << "[+] up to date (v" << kAppVersion << ")\n";
        return false;
    }

    std::cout << "[*] update available: v" << remoteVer << "  (you have v" << kAppVersion << ")\n";
    if (!LooksLikeUrl(remoteExe))
    {
        std::cout << "[!] no download URL - continuing without update\n";
        return false;
    }

    std::cout << "[*] force-closing other robloxini instances...\n";
    const int killed = ForceKillOtherInstances();
    if (killed > 0)
        std::cout << "[+] closed " << killed << " other instance(s)\n";

    std::cout << "[*] downloading update...\n";
    std::cout << "[*] " << remoteExe << "\n";

    const auto exePath = ThisExePath();
    const auto dir = exePath.parent_path();
    const auto tempDl = dir / "robloxini_update_dl.bin";
    const auto tempExe = dir / "robloxini_update.exe";
    const auto tempZip = dir / "robloxini_update.zip";
    const auto batPath = dir / "robloxini_apply_update.bat";

    std::error_code ec;
    std::filesystem::remove(tempDl, ec);
    std::filesystem::remove(tempExe, ec);
    std::filesystem::remove(tempZip, ec);

    if (!DownloadFile(remoteExe, tempDl))
    {
        std::cout << "[!] download failed - continuing without update\n";
        return false;
    }

    bool haveNewExe = false;
    if (IsPeExecutable(tempDl))
    {
        std::filesystem::rename(tempDl, tempExe, ec);
        if (ec)
        {
            std::filesystem::copy_file(tempDl, tempExe, std::filesystem::copy_options::overwrite_existing, ec);
            std::filesystem::remove(tempDl, ec);
        }
        haveNewExe = IsPeExecutable(tempExe);
    }
    else if (IsZipFile(tempDl))
    {
        std::filesystem::rename(tempDl, tempZip, ec);
        if (ec)
            std::filesystem::copy_file(tempDl, tempZip, std::filesystem::copy_options::overwrite_existing, ec);

        std::wstring ps =
            L"powershell -NoProfile -ExecutionPolicy Bypass -Command \""
            L"Expand-Archive -LiteralPath '" + tempZip.wstring() + L"' -DestinationPath '" +
            (dir / "robloxini_update_extract").wstring() + L"' -Force\"";
        STARTUPINFOW siPs{};
        siPs.cb = sizeof(siPs);
        PROCESS_INFORMATION piPs{};
        std::vector<wchar_t> cmdPs(ps.begin(), ps.end());
        cmdPs.push_back(0);
        if (CreateProcessW(nullptr, cmdPs.data(), nullptr, nullptr, FALSE,
                CREATE_NO_WINDOW, nullptr, dir.wstring().c_str(), &siPs, &piPs))
        {
            WaitForSingleObject(piPs.hProcess, 60000);
            CloseHandle(piPs.hThread);
            CloseHandle(piPs.hProcess);
        }

        const auto extractDir = dir / "robloxini_update_extract";
        if (std::filesystem::exists(extractDir))
        {
            for (auto& ent : std::filesystem::recursive_directory_iterator(extractDir, ec))
            {
                if (!ent.is_regular_file()) continue;
                auto ext = ent.path().extension().wstring();
                for (auto& c : ext) c = (wchar_t)towlower(c);
                if (ext == L".exe" && IsPeExecutable(ent.path()))
                {
                    std::filesystem::copy_file(ent.path(), tempExe, std::filesystem::copy_options::overwrite_existing, ec);
                    haveNewExe = !ec && IsPeExecutable(tempExe);
                    break;
                }
            }
            std::filesystem::remove_all(extractDir, ec);
        }
        std::filesystem::remove(tempZip, ec);
        std::filesystem::remove(tempDl, ec);
    }
    else
    {
        std::cout << "[!] downloaded file is not an exe or zip - continuing\n";
        std::filesystem::remove(tempDl, ec);
        return false;
    }

    if (!haveNewExe)
    {
        std::cout << "[!] could not prepare new exe - continuing without update\n";
        return false;
    }

    std::error_code szEc;
    const auto newSz = std::filesystem::file_size(tempExe, szEc);
    if (szEc || newSz < 200000)
    {
        std::cout << "[!] new exe looks too small - continuing without update\n";
        std::filesystem::remove(tempExe, ec);
        return false;
    }

    std::ofstream bat(batPath, std::ios::trunc);
    if (!bat)
    {
        std::cout << "[!] could not write updater script\n";
        std::filesystem::remove(tempExe, ec);
        return false;
    }

    const std::string exe = exePath.string();
    const std::string tmp = tempExe.string();

    bat << "@echo off\r\n";
    bat << "setlocal\r\n";
    bat << "timeout /t 1 /nobreak >nul\r\n";
    bat << "taskkill /F /IM robloxini.exe >nul 2>&1\r\n";
    bat << "timeout /t 1 /nobreak >nul\r\n";
    bat << "if not exist \"" << tmp << "\" goto fail\r\n";
    bat << "set /a n=0\r\n";
    bat << ":retry\r\n";
    bat << "set /a n+=1\r\n";
    bat << "if %n% GTR 40 goto fail\r\n";
    bat << "copy /y \"" << tmp << "\" \"" << exe << "\" >nul\r\n";
    bat << "if errorlevel 1 (\r\n";
    bat << "  taskkill /F /IM robloxini.exe >nul 2>&1\r\n";
    bat << "  timeout /t 1 /nobreak >nul\r\n";
    bat << "  goto retry\r\n";
    bat << ")\r\n";
    bat << "if not exist \"" << exe << "\" goto fail\r\n";
    bat << "del /f /q \"" << tmp << "\" >nul 2>&1\r\n";
    bat << "(\r\n";
    bat << "echo # robloxini update channel\r\n";
    bat << "echo version=https://api.github.com/repos/bellly1/Nyx-external/contents/version.txt\r\n";
    bat << "echo exe=https://raw.githubusercontent.com/bellly1/Nyx-external/main/Nyx.bin\r\n";
    bat << ") > \"%~dp0update_urls.txt\"\r\n";
    bat << "start \"\" \"" << exe << "\"\r\n";
    bat << "del /f /q \"%~f0\" >nul 2>&1\r\n";
    bat << "exit /b 0\r\n";
    bat << ":fail\r\n";
    bat << "echo update failed > \"%~dp0robloxini_update_error.txt\"\r\n";
    bat << "if exist \"" << tmp << "\" copy /y \"" << tmp << "\" \"" << exe << "\" >nul 2>&1\r\n";
    bat << "del /f /q \"%~f0\" >nul 2>&1\r\n";
    bat.close();

    WriteUpdateUrlsFile(dir);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"cmd.exe /C \"" + batPath.wstring() + L"\"";
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(0);

    if (!CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, dir.wstring().c_str(), &si, &pi))
    {
        std::cout << "[!] could not start updater\n";
        std::filesystem::remove(tempExe, ec);
        std::filesystem::remove(batPath, ec);
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    std::cout << "[+] applying update to v" << remoteVer << " - restarting...\n";
    return true;
}

}
