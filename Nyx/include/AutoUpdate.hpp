#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>

namespace AutoUpdate
{

inline constexpr const char* kAppVersion = "1.0";

inline constexpr const char* kBrowserUA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36";

inline constexpr const char* kDefaultVersionUrl =
    "https://raw.githubusercontent.com/bellly1/Nyx-external/main/version.txt";
inline constexpr const char* kDefaultExeUrl =
    "https://raw.githubusercontent.com/bellly1/Nyx-external/main/Nyx.bin";

inline constexpr const char* kDefaultStatusUrl =
    "https://api.github.com/repos/bellly1/Nyx-external/contents/status.txt";
inline constexpr const char* kStatusFallbacks[] = {
    "https://api.github.com/repos/bellly1/Nyx-external/contents/status.txt",
    "https://raw.githubusercontent.com/bellly1/Nyx-external/main/status.txt",
    "https://cdn.jsdelivr.net/gh/bellly1/Nyx-external@main/status.txt",
};

inline constexpr const char* kVersionFallbacks[] = {
    "https://github.com/bellly1/Nyx-external/contents/version.txt",
    "https://raw.githubusercontent.com/bellly1/Nyx-external/main/version.txt",
    "https://cdn.jsdelivr.net/gh/bellly1/Nyx-external@main/version.txt",
};

inline std::string g_versionUrl = kDefaultVersionUrl;
inline std::string g_exeUrl = kDefaultExeUrl;

inline int VersionToInt(const std::string& v)
{
    int major = 0, minor = 0, patch = 0;
    const char* p = v.c_str();
    while (*p && (*p < '0' || *p > '9')) ++p;
    if (*p) major = std::atoi(p);
    while (*p && *p != '.') ++p;
    if (*p == '.') { ++p; minor = std::atoi(p); }
    while (*p && *p != '.') ++p;
    if (*p == '.') { ++p; patch = std::atoi(p); }
    return major * 1000000 + minor * 1000 + patch;
}

inline bool RemoteIsNewer(const std::string& remote, const std::string& local)
{
    if (remote.empty() || local.empty()) return false;
    if (remote == local) return false;
    const int r = VersionToInt(remote);
    const int l = VersionToInt(local);
    if (r > 0 && l > 0)
        return r > l;
    return false;
}

inline std::filesystem::path ThisExePath()
{
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf);
}

inline std::string Trim(std::string s)
{
    while (!s.empty() && (unsigned char)s.back() <= ' ') s.pop_back();
    size_t i = 0;
    while (i < s.size() && (unsigned char)s[i] <= ' ') ++i;
    return s.substr(i);
}

inline std::wstring ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0)
        n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    if (n > 0)
    {
        if (!MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n))
            MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), w.data(), n);
    }
    return w;
}

inline bool LooksLikeUrl(const std::string& s)
{
    return s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0;
}

inline bool LooksLikeHtml(const std::string& s)
{
    if (s.size() < 9) return false;
    const size_t n = (std::min)(s.size(), size_t(512));
    std::string head = s.substr(0, n);
    for (auto& c : head) c = (char)std::tolower((unsigned char)c);
    if (head.find("<!doctype") != std::string::npos) return true;
    if (head.find("<html") != std::string::npos) return true;
    if (head.find("<head") != std::string::npos) return true;
    if (head.find("please read") != std::string::npos) return true;
    if (head.find("filebin") != std::string::npos && head.find("<") != std::string::npos) return true;
    return false;
}

inline bool ReadFileBytes(const std::filesystem::path& path, std::vector<char>& out)
{
    out.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return !out.empty();
}

inline std::filesystem::path MakeTempFile(const char* prefix)
{
    char dir[MAX_PATH]{};
    char file[MAX_PATH]{};
    GetTempPathA(MAX_PATH, dir);
    GetTempFileNameA(dir, prefix, 0, file);
    return std::filesystem::path(file);
}

inline bool IsPeExecutable(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    char mz[2]{};
    in.read(mz, 2);
    return mz[0] == 'M' && mz[1] == 'Z';
}

inline bool IsZipFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    char sig[2]{};
    in.read(sig, 2);
    return sig[0] == 'P' && sig[1] == 'K';
}

inline std::string WithCacheBust(const std::string& url)
{
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (url.find('?') != std::string::npos)
        return url + "&_=" + std::to_string(ms);
    return url + "?_=" + std::to_string(ms);
}

void LoadUrlOverrides();
bool DownloadFile(const std::string& url, const std::filesystem::path& dest);
bool CheckRemoteDisable(bool quiet = false);
bool CheckAndUpdate();

}
