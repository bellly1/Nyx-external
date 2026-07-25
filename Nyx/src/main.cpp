#include "Memory.hpp"
#include "Engine.hpp"
#include "Overlay.hpp"
#include "Offsets.hpp"
#include "Config.hpp"
#include "AutoUpdate.hpp"
#include "Protect.hpp"
#include "Fonts.hpp"
#include "security/SecurityRuntime.hpp"
#include "security/HwId.hpp"
#include "security/ConfigCrypto.hpp"

#include <easyauth/auth.hpp>
#include <easyauth/credentials.hpp>

#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <filesystem>
#include <cctype>

namespace
{
std::string PcUsername()
{
    char buf[256]{};
    DWORD n = static_cast<DWORD>(sizeof(buf));
    if (GetUserNameA(buf, &n) && buf[0])
        return buf;
    return XS("User");
}

std::string ReadLine()
{
    std::string line;
    std::getline(std::cin, line);
    while (!line.empty() && static_cast<unsigned char>(line.back()) <= ' ')
        line.pop_back();
    size_t i = 0;
    while (i < line.size() && static_cast<unsigned char>(line[i]) <= ' ')
        ++i;
    return line.substr(i);
}

std::wstring ProcessNameW()
{
    const std::string a = XS("RobloxPlayerBeta.exe");
    return std::wstring(a.begin(), a.end());
}

bool KeyLooksValid(const std::string& key)
{
    if (key.size() < 8) return false;
    int n = 0;
    for (unsigned char c : key)
        if (std::isalnum(c) || c == '-' || c == '_') ++n;
    return n >= 8;
}

std::string FriendlyAuthError(const std::string& msg)
{
    std::string m = msg;
    for (char& c : m) c = (char)std::tolower((unsigned char)c);
    if (m.find("network") != std::string::npos || m.find("connect") != std::string::npos
        || m.find("timeout") != std::string::npos || m.find("tls") != std::string::npos)
        return "connection failed";
    if (m.find("invalid") != std::string::npos || m.find("expired") != std::string::npos
        || m.find("key") != std::string::npos || m.find("license") != std::string::npos
        || m.find("denied") != std::string::npos || m.find("banned") != std::string::npos)
        return "invalid or expired license key";
    if (m.find("signature") != std::string::npos || m.find("verify") != std::string::npos)
        return "security verification failed";
    if (!msg.empty()) return "authentication failed";
    return "authentication failed";
}

std::string FormatExpiry(long long unixSec)
{
    if (unixSec <= 0) return "lifetime";
    const time_t t = static_cast<time_t>(unixSec);
    struct tm tm{};
    localtime_s(&tm, &t);
    char buf[64]{};
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
    return buf;
}

// Local encrypted license-key cache (key-only system — no username/password)
std::filesystem::path KeyCachePath()
{
    wchar_t mod[MAX_PATH]{};
    GetModuleFileNameW(nullptr, mod, MAX_PATH);
    return std::filesystem::path(mod).parent_path() / L".lic_key";
}

bool LoadSavedKey(std::string& out)
{
    out.clear();
    std::ifstream in(KeyCachePath(), std::ios::binary);
    if (!in) return false;
    std::string blob((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (blob.empty()) return false;
    out = Security::ConfigCrypto::Unprotect(blob);
    return KeyLooksValid(out);
}

bool SaveKey(const std::string& key)
{
    if (!KeyLooksValid(key)) return false;
    std::ofstream out(KeyCachePath(), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    const std::string blob = Security::ConfigCrypto::Protect(key);
    out.write(blob.data(), (std::streamsize)blob.size());
    return true;
}

void ClearSavedKey()
{
    std::error_code ec;
    std::filesystem::remove(KeyCachePath(), ec);
}
} // namespace

int main()
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleTitleA("Nyx V1");

    const std::string pcUser = PcUsername();
    std::cout << "  Nyx  v" << AutoUpdate::kAppVersion << "\n";
    std::cout << "  welcome, " << pcUser << "\n\n";

    Protect::Bootstrap();
    Protect::JunkCode();
    Security::SecurityRuntime::instance().start();

    if (!Harden::Alive())
    {
        std::cout << "[!] security check failed (close debuggers / only one instance)\n";
        std::cout << "press enter to exit...";
        std::cin.get();
        Security::SecurityRuntime::instance().stop();
        Protect::StopProtectThread();
        return 1;
    }

    if (AutoUpdate::CheckAndUpdate())
    {
        Security::SecurityRuntime::instance().stop();
        Protect::StopProtectThread();
        return 0;
    }

    if (!Protect::OpaqueTrue())
    {
        Security::SecurityRuntime::instance().stop();
        Protect::StopProtectThread();
        return 0;
    }
    Protect::JunkCode();

    // ---------- KEY-ONLY license (EasyAuth RedeemKey — no user/pass) ----------
    easyauth::init();
    easyauth::Config eaCfg;
    eaCfg.app_id = EASYAUTH_APP_ID;
    eaCfg.server_public_key_b64 = EASYAUTH_PUBLIC_KEY_B64;
    eaCfg.app_secret_b64 = EASYAUTH_APP_SECRET_B64;
    eaCfg.timeout_ms = 15000;
    easyauth::Client auth(eaCfg);

    std::cout << "[*] connecting...\n";
    bool authed = false;
    std::string sessionToken;
    long long expiresAt = 0;
    std::string key;

    auto applyOk = [&](const easyauth::Result& r) {
        sessionToken = !r.token.empty() ? r.token : auth.GetHWID();
        expiresAt = r.expires_at ? r.expires_at : auth.ExpiresAt();
        authed = true;
    };

    // 1) Saved key → redeem with server (true key system, not user/pass)
    if (LoadSavedKey(key))
    {
        std::cout << "[*] verifying key...\n";
        auto r = auth.RedeemKey(key);
        if (r.ok && auth.IsAuthenticated())
        {
            applyOk(r);
            std::cout << "[+] license ok\n";
        }
        else
        {
            // Session may still be valid if key already bound to this HWID
            if (auth.LoadSession())
            {
                auto v = auth.Validate();
                if (v.ok && auth.IsAuthenticated())
                {
                    applyOk(v);
                    std::cout << "[+] license ok\n";
                }
            }
            if (!authed)
            {
                std::cout << "[!] saved key rejected: " << FriendlyAuthError(r.error) << "\n";
                ClearSavedKey();
                auth.ClearSession();
                Protect::Wipe(key);
                key.clear();
            }
        }
    }

    // 2) Prompt for key only — open LootLabs when no active license
    if (!authed)
    {
        ShellExecuteA(
            nullptr,
            "open",
            "https://work.ink/2KNL/nyx-1-day-key",
            nullptr,
            nullptr,
            SW_SHOWNORMAL);
        std::cout << "[*] browser opened — finish the locker for a key\n";
        std::cout << "paste your license key and press enter:\n> ";
        key = ReadLine();
        if (!KeyLooksValid(key))
        {
            std::cout << "[!] that is not a valid license key\n";
            Protect::Wipe(key);
            std::cout << "press enter to exit...";
            std::cin.get();
            Security::SecurityRuntime::instance().stop();
            Protect::StopProtectThread();
            return 1;
        }

        std::cout << "[*] activating key...\n";
        auto r = auth.RedeemKey(key);
        if (!r.ok || !auth.IsAuthenticated())
        {
            std::cout << "[!] " << FriendlyAuthError(r.error) << "\n";
            auth.ClearSession();
            Protect::Wipe(key);
            ClearSavedKey();
            std::cout << "press enter to exit...";
            std::cin.get();
            Security::SecurityRuntime::instance().stop();
            Protect::StopProtectThread();
            return 1;
        }

        SaveKey(key);
        applyOk(r);
        std::cout << "[+] license activated\n";
    }

    // Final live check
    {
        auto v = auth.Validate();
        if (!v.ok || !auth.IsAuthenticated())
        {
            std::cout << "[!] auth rejected\n";
            Protect::BurnSeal();
            auth.ClearSession();
            std::cout << "press enter to exit...";
            std::cin.get();
            Security::SecurityRuntime::instance().stop();
            Protect::StopProtectThread();
            return 1;
        }
        if (!v.token.empty())
            sessionToken = v.token;
        if (expiresAt <= 0)
            expiresAt = v.expires_at ? v.expires_at : auth.ExpiresAt();
    }

    Protect::Wipe(key);

    {
        std::string hwid = auth.GetHWID();
        if (hwid.empty())
            hwid = Security::HwId::Collect();

        // Always mint a non-empty session material so the license gate stays armed
        std::string gateToken = sessionToken;
        if (gateToken.empty())
            gateToken = std::string("nyx-key|") + hwid;
        const std::string sealId = gateToken + "|key";
        Protect::MintSeal(sealId, hwid);
        Protect::LockRuntime(gateToken, hwid);
        Protect::Wipe(hwid);
        Protect::Wipe(gateToken);
        Protect::JunkCode();

        if (!Protect::Seal().live.load() || !Protect::LicensedHot())
        {
            std::cout << "[!] license gate failed — re-run and activate key again\n";
            Protect::BurnSeal();
            Security::SecurityRuntime::instance().stop();
            Protect::StopProtectThread();
            return 1;
        }
        std::cout << "[+] combat unlocked\n";
    }

    Protect::HideThreadFromDebugger();

    if (expiresAt > 0)
        std::cout << "[+] expires: " << FormatExpiry(expiresAt) << "\n";
    else
        std::cout << "[+] expires: lifetime\n";

    Fonts::EnsureRobotoDownloaded();
    std::cout << "\n";

    std::atomic<bool> authWatch{ true };
    std::thread authThread([&]() {
        int sessionFails = 0;
        while (authWatch.load(std::memory_order_acquire))
        {
            for (int i = 0; i < 15 && authWatch.load(); ++i)
                std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!authWatch.load())
                break;

            if (!Protect::Seal().live.load(std::memory_order_relaxed))
            {
                std::cout << "\n[!] license lost — shutting down\n";
                authWatch.store(false);
                break;
            }

            // Server re-check (signed response)
            if (auth.IsAuthenticated())
                sessionFails = 0;
            else
                ++sessionFails;

            if (sessionFails >= 8)
            {
                std::cout << "\n[!] session expired — shutting down\n";
                Protect::OnAuthFailure();
                authWatch.store(false);
                break;
            }
        }
    });

    std::cout << "[*] updating offsets...\n";
    if (Offsets::FetchOffsets())
        std::cout << "[+] offsets ready  |  " << Offsets::ClientVersion << "\n\n";
    else
        std::cout << "[!] offset fetch failed\n\n";

    Memory memory;
    const std::wstring proc = ProcessNameW();
    std::cout << "[*] waiting for game...\n";
    while (!memory.Attach(proc.c_str()))
    {
        if (!authWatch.load())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!authWatch.load() && !memory.GetProcessId())
    {
        if (authThread.joinable())
            authThread.join();
        Security::SecurityRuntime::instance().stop();
        Protect::StopProtectThread();
        return 1;
    }

    std::cout << "[+] attached  pid=" << memory.GetProcessId() << "\n";
    const uintptr_t base = memory.GetModuleBase(proc.c_str());
    if (!base)
        std::cout << "[!] module base failed - try Run as Administrator\n";
    else
        std::cout << "[+] module base ok\n";

    std::cout << "[*] menu: Right Shift\n\n";

    Engine engine(memory);
    engine.Start();
    if (!authWatch.load())
        engine.running.store(false, std::memory_order_release);

    std::atomic<bool> killWatch{ true };
    std::thread killThread([&]() {
        while (killWatch.load(std::memory_order_acquire)
            && engine.running.load(std::memory_order_acquire))
        {
            for (int i = 0; i < 4 && killWatch.load() && engine.running.load(); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            if (!killWatch.load() || !engine.running.load())
                break;

            if (!memory.IsProcessAlive())
            {
                std::cout << "\n[*] game closed — shutting down\n";
                engine.running.store(false, std::memory_order_release);
                break;
            }

            if (AutoUpdate::CheckRemoteDisable(true))
            {
                std::cout << "\n[!] remote disable — shutting down\n";
                engine.running.store(false, std::memory_order_release);
                break;
            }
            if (!authWatch.load())
            {
                engine.running.store(false, std::memory_order_release);
                break;
            }
        }
    });

    for (int i = 0; i < 50; ++i)
    {
        if (auto g = engine.Globals(); g && g->valid)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Overlay overlay(memory, engine);
    if (!overlay.Start())
    {
        engine.running.store(false, std::memory_order_release);
        killWatch.store(false);
        authWatch.store(false);
        if (killThread.joinable())
            killThread.join();
        if (authThread.joinable())
            authThread.join();
        engine.Stop();
        FreeConsole();
        Security::SecurityRuntime::instance().stop();
        Protect::StopProtectThread();
        return 1;
    }

    while (engine.running.load(std::memory_order_acquire))
    {
        if (!authWatch.load() || !memory.IsProcessAlive())
        {
            if (!memory.IsProcessAlive())
                std::cout << "\n[*] game closed — shutting down\n";
            engine.running.store(false, std::memory_order_release);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    killWatch.store(false);
    authWatch.store(false);
    if (killThread.joinable())
        killThread.join();
    if (authThread.joinable())
        authThread.join();

    overlay.Stop();
    overlay.Join();
    engine.Stop();

    FreeConsole();
    Protect::BurnSeal();
    Security::SecurityRuntime::instance().stop();
    Protect::StopProtectThread();
    return 0;
}
