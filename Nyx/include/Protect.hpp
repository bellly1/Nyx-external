#pragma once

#include "skStr.h"
#include "Harden.hpp"
#include "security/LicenseGate.hpp"
#include "security/SecurityRuntime.hpp"
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <thread>
#include <intrin.h>

namespace Protect
{

// ---------------------------------------------------------------------------
// Memory wipe
// ---------------------------------------------------------------------------
inline void Wipe(std::string& s)
{
    if (!s.empty())
    {
        SecureZeroMemory(s.data(), s.size());
        s.clear();
        s.shrink_to_fit();
    }
}

inline void Wipe(char* p, size_t n)
{
    if (p && n) SecureZeroMemory(p, n);
}

inline void Wipe(wchar_t* p, size_t nChars)
{
    if (p && nChars) SecureZeroMemory(p, nChars * sizeof(wchar_t));
}

// Decrypt compile-time string into std::string
#define XS(str) (std::string(skCrypt(str).decrypt()))

// ---------------------------------------------------------------------------
// Hash / salts
// ---------------------------------------------------------------------------
inline uint64_t Fnv1a64(const void* data, size_t len, uint64_t seed = 14695981039346656037ull)
{
    const auto* p = static_cast<const uint8_t*>(data);
    uint64_t h = seed;
    for (size_t i = 0; i < len; ++i)
    {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdull;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ull;
    h ^= h >> 33;
    return h;
}

inline uint64_t Fnv1a64(const std::string& s, uint64_t seed = 14695981039346656037ull)
{
    return Fnv1a64(s.data(), s.size(), seed);
}

inline uint64_t SaltA()
{
    const std::string s = XS("obs.seal.v6.alpha.a91c");
    return Fnv1a64(s);
}
inline uint64_t SaltB()
{
    const std::string s = XS("obs.seal.v6.beta.e40f");
    return Fnv1a64(s);
}
inline uint64_t SaltC()
{
    const std::string s = XS("obs.seal.v6.gamma.77b2");
    return Fnv1a64(s);
}
inline uint64_t SaltD()
{
    const std::string s = XS("obs.seal.v6.delta.3c88");
    return Fnv1a64(s);
}

// ---------------------------------------------------------------------------
// NT helpers (resolved at runtime — names never as plain imports where possible)
// ---------------------------------------------------------------------------
using NtQueryInformationProcess_t = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
using NtSetInformationThread_t    = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG);
using NtQuerySystemInformation_t  = LONG(NTAPI*)(ULONG, PVOID, ULONG, PULONG);
using NtClose_t                   = LONG(NTAPI*)(HANDLE);

inline HMODULE Ntdll()
{
    static HMODULE h = nullptr;
    if (!h)
        h = GetModuleHandleA(skCrypt("ntdll.dll").decrypt());
    return h;
}

inline void* NtProc(const char* name)
{
    HMODULE n = Ntdll();
    return n ? reinterpret_cast<void*>(GetProcAddress(n, name)) : nullptr;
}

// ---------------------------------------------------------------------------
// Anti-debug primitives
// ---------------------------------------------------------------------------
inline bool QueryDebugPort()
{
#if !defined(NDEBUG)
    return false;
#else
    auto fn = reinterpret_cast<NtQueryInformationProcess_t>(
        NtProc(skCrypt("NtQueryInformationProcess").decrypt()));
    if (!fn) return false;

    ULONG_PTR port = 0;
    if (fn(GetCurrentProcess(), 7, &port, sizeof(port), nullptr) == 0 && port != 0)
        return true;

    ULONG flags = 1;
    if (fn(GetCurrentProcess(), 0x1f, &flags, sizeof(flags), nullptr) == 0 && flags == 0)
        return true;

    HANDLE obj = nullptr;
    if (fn(GetCurrentProcess(), 0x1e, &obj, sizeof(obj), nullptr) == 0 && obj)
    {
        CloseHandle(obj);
        return true;
    }

    // ProcessBasicInformation → InheritedFromUniqueProcessId checks done elsewhere
    return false;
#endif
}

inline bool HardwareBreakpoints()
{
#if !defined(NDEBUG)
    return false;
#else
    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(GetCurrentThread(), &ctx))
        return false;
    return (ctx.Dr0 | ctx.Dr1 | ctx.Dr2 | ctx.Dr3) != 0;
#endif
}

inline bool KernelDebuggerPresent()
{
#if !defined(NDEBUG)
    return false;
#else
    auto fn = reinterpret_cast<NtQuerySystemInformation_t>(
        NtProc(skCrypt("NtQuerySystemInformation").decrypt()));
    if (!fn) return false;

    // SystemKernelDebuggerInformation = 0x23
    struct { BOOLEAN DebuggerEnabled; BOOLEAN DebuggerNotPresent; } info{};
    if (fn(0x23, &info, sizeof(info), nullptr) != 0)
        return false;
    return info.DebuggerEnabled && !info.DebuggerNotPresent;
#endif
}

inline bool PebDebugFlags()
{
#if !defined(NDEBUG)
    return false;
#else
#ifdef _WIN64
    const auto peb = reinterpret_cast<const unsigned char*>(__readgsqword(0x60));
    if (!peb) return false;
    // BeingDebugged
    if (peb[2]) return true;
    // NtGlobalFlag (0xBC): FLG_HEAP_ENABLE_TAIL_CHECK | FREE_CHECK | VALIDATE_PARAMETERS
    const auto ntGlobalFlag = *reinterpret_cast<const unsigned long*>(peb + 0xBC);
    if (ntGlobalFlag & 0x70) return true;
#else
    const auto peb = reinterpret_cast<const unsigned char*>(__readfsdword(0x30));
    if (!peb) return false;
    if (peb[2]) return true;
    const auto ntGlobalFlag = *reinterpret_cast<const unsigned long*>(peb + 0x68);
    if (ntGlobalFlag & 0x70) return true;
#endif
    return false;
#endif
}

// Soft signal — heap flags alone often false-positive under modern CRT
inline bool HeapFlagsSuspicious()
{
#if !defined(NDEBUG)
    return false;
#else
#ifdef _WIN64
    const auto peb = reinterpret_cast<const unsigned char*>(__readgsqword(0x60));
    if (!peb) return false;
    const auto heap = *reinterpret_cast<const uintptr_t*>(peb + 0x30);
    if (!heap) return false;
    const auto flags = *reinterpret_cast<const unsigned long*>(heap + 0x70);
    const auto force = *reinterpret_cast<const unsigned long*>(heap + 0x74);
    return ((flags & ~0x2u) != 0) || (force != 0);
#else
    return false;
#endif
#endif
}

inline bool CloseHandleExceptionTrick()
{
#if !defined(NDEBUG)
    return false;
#else
    // Invalid handle under a debugger may raise EXCEPTION_INVALID_HANDLE
    __try
    {
        CloseHandle(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(0xDEADBEEF)));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return true;
    }
    return false;
#endif
}

inline bool OutputDebugStringTrick()
{
#if !defined(NDEBUG)
    return false;
#else
    SetLastError(0);
    OutputDebugStringA(skCrypt(".").decrypt());
    // Under some debuggers last error is altered
    return GetLastError() != 0 && IsDebuggerPresent();
#endif
}

inline bool TimingCheck()
{
#if !defined(NDEBUG)
    return false;
#else
    const auto t0 = __rdtsc();
    volatile int x = 0;
    for (int i = 0; i < 800; ++i)
        x += i * 3 + (i ^ 0x5a);
    const auto t1 = __rdtsc();
    const uint64_t dt = t1 - t0;
    // Single-step / heavy instrumentation blows this up
    if (dt > 25000000ull)
        return true;

    const auto c0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 400; ++i) x += i;
    const auto c1 = std::chrono::high_resolution_clock::now();
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(c1 - c0).count();
    (void)x;
    return us > 45000;
#endif
}

inline bool ParentIsDebugger()
{
#if !defined(NDEBUG)
    return false;
#else
    auto fn = reinterpret_cast<NtQueryInformationProcess_t>(
        NtProc(skCrypt("NtQueryInformationProcess").decrypt()));
    if (!fn) return false;

    struct PROCESS_BASIC_INFORMATION_L {
        PVOID Reserved1;
        PVOID PebBaseAddress;
        PVOID Reserved2[2];
        ULONG_PTR UniqueProcessId;
        ULONG_PTR InheritedFromUniqueProcessId;
    } pbi{};

    if (fn(GetCurrentProcess(), 0, &pbi, sizeof(pbi), nullptr) != 0)
        return false;

    const DWORD ppid = static_cast<DWORD>(pbi.InheritedFromUniqueProcessId);
    if (!ppid) return false;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    bool bad = false;
    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (pe.th32ProcessID != ppid) continue;
            char nameA[MAX_PATH]{};
            WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, nameA, MAX_PATH, nullptr, nullptr);
            for (char* p = nameA; *p; ++p)
                *p = static_cast<char>(::tolower(static_cast<unsigned char>(*p)));

            // Common debug / analysis parents
            if (std::strstr(nameA, skCrypt("x64dbg").decrypt())
                || std::strstr(nameA, skCrypt("x32dbg").decrypt())
                || std::strstr(nameA, skCrypt("ollydbg").decrypt())
                || std::strstr(nameA, skCrypt("ida64").decrypt())
                || std::strstr(nameA, skCrypt("windbg").decrypt())
                || std::strstr(nameA, skCrypt("cheatengine").decrypt()))
            {
                bad = true;
            }
            break;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return bad;
#endif
}

inline bool SuspiciousWindowPresent()
{
#if !defined(NDEBUG)
    return false;
#else
    struct Ctx { bool hit; } ctx{ false };
    EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
        auto* c = reinterpret_cast<Ctx*>(lp);
        if (!IsWindowVisible(hwnd)) return TRUE;
        char title[256]{};
        if (!GetWindowTextA(hwnd, title, 256) || !title[0]) return TRUE;
        for (char* p = title; *p; ++p)
            *p = static_cast<char>(::tolower(static_cast<unsigned char>(*p)));

        if (std::strstr(title, skCrypt("x64dbg").decrypt())
            || std::strstr(title, skCrypt("x32dbg").decrypt())
            || std::strstr(title, skCrypt("ollydbg").decrypt())
            || std::strstr(title, skCrypt("immunity").decrypt())
            || std::strstr(title, skCrypt("ida -").decrypt())
            || std::strstr(title, skCrypt("ida pro").decrypt())
            || std::strstr(title, skCrypt("ghidra").decrypt())
            || std::strstr(title, skCrypt("cheat engine").decrypt())
            || std::strstr(title, skCrypt("process hacker").decrypt())
            || std::strstr(title, skCrypt("http debugger").decrypt())
            || std::strstr(title, skCrypt("fiddler").decrypt())
            || std::strstr(title, skCrypt("wireshark").decrypt())
            || std::strstr(title, skCrypt("dnspy").decrypt()))
        {
            c->hit = true;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&ctx));
    return ctx.hit;
#endif
}

inline bool SuspiciousModuleLoaded()
{
#if !defined(NDEBUG)
    return false;
#else
    // Analysis / hook / dump DLLs often injected into target
    static const char* kMods[] = {
        "vehdebug", "winhook", "hook", "scyllahide", "sharpod", "x64menu",
        "x32menu", "titanengine", "dbghelp.proxy", "api_log", "dir_watch",
        "cmdvrt32", "cmdvrt64", "sbiedll", "aswhook", "snxhk"
    };
    // Compare against loaded modules via Toolhelp
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return false;

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    bool hit = false;
    if (Module32FirstW(snap, &me))
    {
        do
        {
            char nameA[MAX_PATH]{};
            WideCharToMultiByte(CP_UTF8, 0, me.szModule, -1, nameA, MAX_PATH, nullptr, nullptr);
            for (char* p = nameA; *p; ++p)
                *p = static_cast<char>(::tolower(static_cast<unsigned char>(*p)));

            for (const char* bad : kMods)
            {
                // encrypt at use via stack copy from skCrypt for hot names
                if (std::strstr(nameA, bad))
                {
                    hit = true;
                    break;
                }
            }
            if (hit) break;
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return hit;
#endif
}

inline bool SuspiciousProcessRunning()
{
#if !defined(NDEBUG)
    return false;
#else
    // Names decrypted per-check so they never sit as one contiguous plaintext array
    auto match = [](const char* nameA, const char* needle) -> bool {
        return std::strstr(nameA, needle) != nullptr;
    };

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    bool hit = false;
    if (Process32FirstW(snap, &pe))
    {
        do
        {
            char nameA[MAX_PATH]{};
            WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, nameA, MAX_PATH, nullptr, nullptr);
            for (char* p = nameA; *p; ++p)
                *p = static_cast<char>(::tolower(static_cast<unsigned char>(*p)));

            if (match(nameA, skCrypt("x64dbg").decrypt())
                || match(nameA, skCrypt("x32dbg").decrypt())
                || match(nameA, skCrypt("ollydbg").decrypt())
                || match(nameA, skCrypt("ida64").decrypt())
                || match(nameA, skCrypt("ida.exe").decrypt())
                || match(nameA, skCrypt("idaq").decrypt())
                || match(nameA, skCrypt("cheatengine").decrypt())
                || match(nameA, skCrypt("scylla").decrypt())
                || match(nameA, skCrypt("processhacker").decrypt())
                || match(nameA, skCrypt("httpdebugger").decrypt())
                || match(nameA, skCrypt("fiddler").decrypt())
                || match(nameA, skCrypt("wireshark").decrypt())
                || match(nameA, skCrypt("dnspy").decrypt())
                || match(nameA, skCrypt("de4dot").decrypt())
                || match(nameA, skCrypt("ghidra").decrypt())
                || match(nameA, skCrypt("radare2").decrypt())
                || match(nameA, skCrypt("immunitydebugger").decrypt())
                || match(nameA, skCrypt("windbg").decrypt())
                || match(nameA, skCrypt("reshacker").decrypt())
                || match(nameA, skCrypt("petool").decrypt())
                || match(nameA, skCrypt("lordpe").decrypt())
                || match(nameA, skCrypt("importrec").decrypt())
                || match(nameA, skCrypt("megadumper").decrypt())
                || match(nameA, skCrypt("extremedumper").decrypt())
                || match(nameA, skCrypt("hollowshunter").decrypt())
                || match(nameA, skCrypt("keyauthstub").decrypt())
                || match(nameA, skCrypt("keyauth-stub").decrypt())
                || match(nameA, skCrypt("keyauthemulator").decrypt())
                || match(nameA, skCrypt("authstub").decrypt())
                || match(nameA, skCrypt("charles.exe").decrypt())
                || match(nameA, skCrypt("proxifier").decrypt())
                || match(nameA, skCrypt("mitmproxy").decrypt())
                || match(nameA, skCrypt("x64netdumper").decrypt())
                || match(nameA, skCrypt("x64netdumper").decrypt())
                || match(nameA, skCrypt("pe-bear").decrypt())
                || match(nameA, skCrypt("pestudio").decrypt()))
            {
                hit = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return hit;
#endif
}

// Cheap checks only — safe to call every frame
inline bool IsBeingDebuggedLight()
{
#if !defined(NDEBUG)
    return false;
#else
    if (IsDebuggerPresent())
        return true;
    BOOL remote = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote)
        return true;
    return false;
#endif
}

// Heavy scan (process list / windows) — CACHED so menu/ESP stay smooth
inline int DebugThreatScoreHeavy()
{
#if !defined(NDEBUG)
    return 0;
#else
    int score = 0;
    if (QueryDebugPort())
        score += 100;
    if (KernelDebuggerPresent())
        score += 100;
    // Real RE tools only (not procexp/procmon — those false-positive constantly)
    if (SuspiciousProcessRunning())
        score += 100;
    if (SuspiciousWindowPresent())
        score += 80;
    return score;
#endif
}

inline int DebugThreatScore()
{
#if !defined(NDEBUG)
    return 0;
#else
    int score = 0;
    if (IsDebuggerPresent())
        score += 100;
    BOOL remote = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote)
        score += 100;
    if (QueryDebugPort())
        score += 100;
    if (KernelDebuggerPresent())
        score += 100;
    // Soft / noisy checks removed from kill path (timing, heap, closehandle, peb flags)
    // They caused random exits and frame hitches under load
    return score + DebugThreatScoreHeavy();
#endif
}

inline bool IsBeingDebugged()
{
#if !defined(NDEBUG)
    return false;
#else
    if (IsBeingDebuggedLight())
        return true;

    // Cache heavy work ~3s — full process snapshot every frame was freezing the menu
    static ULONGLONG s_last = 0;
    static int s_score = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - s_last >= 3000ull)
    {
        s_last = now;
        s_score = DebugThreatScoreHeavy();
    }
    return s_score >= 100;
#endif
}

// ---------------------------------------------------------------------------
// Process hardening
// ---------------------------------------------------------------------------
inline void HideThreadFromDebugger(HANDLE thread = GetCurrentThread())
{
#if defined(NDEBUG)
    auto fn = reinterpret_cast<NtSetInformationThread_t>(
        NtProc(skCrypt("NtSetInformationThread").decrypt()));
    if (!fn) return;
    // ThreadHideFromDebugger = 0x11
    fn(thread, 0x11, nullptr, 0);
#else
    (void)thread;
#endif
}

inline void DisableDebugPrivilegesBestEffort()
{
#if defined(NDEBUG)
    // Make process slightly less convenient for some attach flows
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
        return;
    // No-op adjust — presence of call + error paths muddies static analysis
    TOKEN_PRIVILEGES tp{};
    AdjustTokenPrivileges(token, FALSE, &tp, 0, nullptr, nullptr);
    CloseHandle(token);
#endif
}

// Optional: wipe PE header in memory (hurts dumpers; can break some tooling)
inline void ErasePeHeader()
{
#if defined(NDEBUG)
    HMODULE mod = GetModuleHandleW(nullptr);
    if (!mod) return;
    auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(mod);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
    auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uint8_t*>(mod) + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return;

    const DWORD headerSize = nt->OptionalHeader.SizeOfHeaders;
    DWORD old = 0;
    if (!VirtualProtect(mod, headerSize, PAGE_READWRITE, &old))
        return;
    SecureZeroMemory(mod, headerSize);
    VirtualProtect(mod, headerSize, old, &old);
#endif
}

inline void HardenProcess()
{
#if defined(NDEBUG)
    HideThreadFromDebugger();
    DisableDebugPrivilegesBestEffort();
    // Mitigate some easy process-info leaks
    SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
#endif
}

inline void DieQuiet()
{
#if defined(NDEBUG)
    volatile uint64_t junk = GetTickCount64() ^ SaltD();
    junk *= 0x9E3779B97F4A7C15ull;
    junk ^= __rdtsc();
    (void)junk;
    // Misleading exit code (access violation style)
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(0xC0000005));
    ExitProcess(0);
    for (;;)
        Sleep(1000);
#endif
}

inline void AntiDebugGate()
{
#if defined(NDEBUG)
    // Only hard attach — full process scan was lagging UI and false-killing
    if (IsBeingDebuggedLight())
        DieQuiet();
#endif
}

// ---------------------------------------------------------------------------
// Opaque predicates / junk
// ---------------------------------------------------------------------------
inline volatile int g_junk = 0;

inline __forceinline int OpaqueTrue()
{
    g_junk ^= static_cast<int>(reinterpret_cast<uintptr_t>(&g_junk));
    g_junk += static_cast<int>(SaltA() & 0xFF);
    const int a = (g_junk | 1);
    const int b = (g_junk ^ g_junk) + 1;
    return (a * b) != 0;
}

inline __forceinline int OpaqueFalsePath()
{
    g_junk ^= 0x5A5A;
    return (g_junk & 0) != 0;
}

inline __forceinline void JunkCode()
{
    volatile uint64_t x = __rdtsc() ^ SaltB();
    x = (x ^ (x << 13)) * 0x9E3779B97F4A7C15ull;
    g_junk ^= static_cast<int>(x);
}

// ---------------------------------------------------------------------------
// License seal
// ---------------------------------------------------------------------------
struct SealState
{
    std::atomic<uint64_t> t0{ 0 };
    std::atomic<uint64_t> t1{ 0 };
    std::atomic<uint64_t> t2{ 0 };
    std::atomic<uint64_t> t3{ 0 };
    std::atomic<uint64_t> nonce{ 0 };
    std::atomic<uint32_t> fail{ 0 };
    std::atomic<uint32_t> ticks{ 0 };
    std::atomic<bool>     live{ false };
    char session[96]{};
    char hwid[96]{};
};

inline SealState& Seal()
{
    static SealState s;
    return s;
}

inline uint64_t WindowNow()
{
    const auto sec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return static_cast<uint64_t>(sec / 30);
}

inline uint64_t DeriveT0(const char* session, const char* hwid, uint64_t nonce)
{
    std::string m;
    m.reserve(256);
    m.append(session ? session : "");
    m.push_back('|');
    m.append(hwid ? hwid : "");
    m.push_back('|');
    m.append(std::to_string(nonce));
    m.push_back('|');
    m.append(std::to_string(SaltA()));
    m.append(std::to_string(GetCurrentProcessId()));
    return Fnv1a64(m) ^ SaltB();
}

inline uint64_t DeriveT1(uint64_t t0, const char* hwid)
{
    std::string m = std::to_string(t0);
    m.push_back('#');
    m.append(hwid ? hwid : "");
    m.append(std::to_string(SaltC()));
    return Fnv1a64(m) ^ (t0 >> 7) ^ SaltD();
}

inline uint64_t DeriveT2(uint64_t t0, uint64_t t1, uint64_t window)
{
    uint64_t x = t0 ^ (t1 * 0x9E3779B97F4A7C15ull) ^ (window * SaltA());
    x ^= x >> 17;
    x *= 0xBF58476D1CE4E5B9ull;
    x ^= x >> 31;
    x ^= SaltD();
    return x;
}

inline uint64_t DeriveT3(uint64_t t0, uint64_t t1, uint64_t t2, uint64_t nonce)
{
    uint64_t x = t0 ^ t1 ^ (t2 << 1) ^ nonce ^ SaltC();
    x *= 0x94D049BB133111EBull;
    x ^= x >> 29;
    return x;
}

inline void MintSeal(const std::string& sessionId, const std::string& hwid)
{
    auto& s = Seal();
    SecureZeroMemory(s.session, sizeof(s.session));
    SecureZeroMemory(s.hwid, sizeof(s.hwid));
    if (!sessionId.empty())
        strncpy_s(s.session, sessionId.c_str(), _TRUNCATE);
    if (!hwid.empty())
        strncpy_s(s.hwid, hwid.c_str(), _TRUNCATE);

    const uint64_t nonce = Fnv1a64(sessionId)
        ^ (GetCurrentProcessId() * 0x100000001B3ull)
        ^ SaltA()
        ^ GetTickCount64()
        ^ __rdtsc();
    s.nonce.store(nonce, std::memory_order_relaxed);

    const uint64_t t0 = DeriveT0(s.session, s.hwid, nonce);
    const uint64_t t1 = DeriveT1(t0, s.hwid);
    const uint64_t t2 = DeriveT2(t0, t1, WindowNow());
    const uint64_t t3 = DeriveT3(t0, t1, t2, nonce);

    s.t0.store(t0, std::memory_order_release);
    s.t1.store(t1, std::memory_order_release);
    s.t2.store(t2, std::memory_order_release);
    s.t3.store(t3, std::memory_order_release);
    s.fail.store(0, std::memory_order_relaxed);
    s.ticks.store(0, std::memory_order_relaxed);
    s.live.store(true, std::memory_order_release);
}

inline void BurnSeal()
{
    // Soft burn — do NOT clear CombatUnlocked so aim keeps working after
    // false-positive anti-tamper ticks. Real exit paths still stop the engine.
    auto& s = Seal();
    s.live.store(false, std::memory_order_release);
    s.t0.store(0, std::memory_order_relaxed);
    s.t1.store(0, std::memory_order_relaxed);
    s.t2.store(0, std::memory_order_relaxed);
    s.t3.store(0, std::memory_order_relaxed);
    s.nonce.store(0, std::memory_order_relaxed);
    SecureZeroMemory(s.session, sizeof(s.session));
    SecureZeroMemory(s.hwid, sizeof(s.hwid));
}

inline bool VerifySealFull()
{
    auto& s = Seal();
    if (!s.live.load(std::memory_order_acquire))
        return false;

    JunkCode();
    AntiDebugGate();

    if (s.session[0] == 0 || s.hwid[0] == 0)
    {
        s.fail.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    const uint64_t nonce = s.nonce.load(std::memory_order_relaxed);
    const uint64_t expect0 = DeriveT0(s.session, s.hwid, nonce);
    const uint64_t expect1 = DeriveT1(expect0, s.hwid);
    const uint64_t w = WindowNow();

    const uint64_t expect2a = DeriveT2(expect0, expect1, w);
    const uint64_t expect2b = DeriveT2(expect0, expect1, w - 1);

    const uint64_t got0 = s.t0.load(std::memory_order_acquire);
    const uint64_t got1 = s.t1.load(std::memory_order_acquire);
    const uint64_t got2 = s.t2.load(std::memory_order_acquire);
    const uint64_t got3 = s.t3.load(std::memory_order_acquire);

    const uint64_t expect3a = DeriveT3(expect0, expect1, expect2a, nonce);
    const uint64_t expect3b = DeriveT3(expect0, expect1, expect2b, nonce);

    if (got0 != expect0 || got1 != expect1
        || (got2 != expect2a && got2 != expect2b)
        || (got3 != expect3a && got3 != expect3b))
    {
        s.fail.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    s.t2.store(expect2a, std::memory_order_release);
    s.t3.store(expect3a, std::memory_order_release);
    return true;
}

// Fast path for every frame / aim tick — seal math only, no process walks
inline bool VerifySealFast()
{
    auto& s = Seal();
    if (!s.live.load(std::memory_order_relaxed))
        return false;

    // Light debugger only (no CreateToolhelp32 every frame)
    if (IsBeingDebuggedLight())
    {
        BurnSeal();
        return false;
    }

    const uint32_t tick = s.ticks.fetch_add(1, std::memory_order_relaxed);

    // Heavy anti-debug at most ~every few hundred ticks (via cache inside IsBeingDebugged)
    if ((tick & 0x1FF) == 0)
    {
        if (IsBeingDebugged())
        {
            BurnSeal();
            return false;
        }
    }

    const uint64_t t0 = s.t0.load(std::memory_order_relaxed);
    const uint64_t t1 = s.t1.load(std::memory_order_relaxed);
    if (!t0 || !t1)
        return false;

    if (DeriveT1(t0, s.hwid) != t1)
    {
        s.fail.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Window token only every ~32 ticks (30s windows — no need every frame)
    if ((tick & 0x1F) == 0)
    {
        const uint64_t w = WindowNow();
        const uint64_t t2 = s.t2.load(std::memory_order_relaxed);
        const uint64_t e2a = DeriveT2(t0, t1, w);
        const uint64_t e2b = DeriveT2(t0, t1, w - 1);
        if (t2 != e2a && t2 != e2b)
        {
            s.fail.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        const uint64_t nonce = s.nonce.load(std::memory_order_relaxed);
        s.t2.store(e2a, std::memory_order_relaxed);
        s.t3.store(DeriveT3(t0, t1, e2a, nonce), std::memory_order_relaxed);
    }
    return true;
}

// Burn seal but do NOT TerminateProcess — lets overlay exit cleanly with a log line
inline void OnAuthFailure()
{
    BurnSeal();
}

inline void OnAuthFailureFatal()
{
    BurnSeal();
    DieQuiet();
}

inline bool Licensed()
{
    if (OpaqueFalsePath())
        return true;
    if (!OpaqueTrue())
        return false;
    // Fail-open a few frames so UI never freezes on a single seal race
    if (!VerifySealFast())
    {
        if (Seal().fail.load(std::memory_order_relaxed) >= 8)
            OnAuthFailure();
        return false;
    }
    Seal().fail.store(0, std::memory_order_relaxed);
    return true;
}

inline bool LicensedStrict()
{
    // Don't DieQuiet on soft anti-debug noise
    if (IsBeingDebuggedLight())
        return false;
    if (!VerifySealFull())
    {
        if (Seal().fail.load() >= 5)
            OnAuthFailure();
        return false;
    }
    return true;
}

// Sticky combat unlock — once key auth succeeds this stays true even if
// Harden/integrity soft-fails later (those were killing aim mid-session).
inline std::atomic<bool>& CombatUnlocked()
{
    static std::atomic<bool> u{ false };
    return u;
}

// Hot path for aim/silent/movement.
inline bool LicensedHot()
{
    if (CombatUnlocked().load(std::memory_order_acquire))
        return true;
    return Seal().live.load(std::memory_order_acquire);
}

// Call after successful server auth + MintSeal
inline void LockRuntime(const std::string& sealSession, const std::string& hwid)
{
    Harden::BindCapability(sealSession.c_str(), hwid.c_str());
    Harden::LockIntegrityBaseline();
    // Modular stack: session capability + integrity lock (server token = sealSession material)
    Security::SecurityRuntime::instance().notifyLicensed(sealSession, hwid);
    CombatUnlocked().store(true, std::memory_order_release);
    Seal().live.store(true, std::memory_order_release);
}

// ---------------------------------------------------------------------------
// Background protect watchdog
// ---------------------------------------------------------------------------
inline std::atomic<bool>& ProtectWatch()
{
    static std::atomic<bool> w{ false };
    return w;
}

inline std::thread& ProtectThreadHandle()
{
    static std::thread t;
    return t;
}

inline void ProtectThreadMain()
{
    HideThreadFromDebugger();
    // Wait until loader is past banner / auth UI before scanning tools
    for (int i = 0; i < 6 && ProtectWatch().load(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    while (ProtectWatch().load(std::memory_order_acquire))
    {
        for (int i = 0; i < 6 && ProtectWatch().load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (!ProtectWatch().load())
            break;

        HideThreadFromDebugger();
        Harden::Tick();

        // Only hard-stop on a real debugger — never burn combat for tool scans
        if (Harden::DebuggerAttachedNow())
        {
            BurnSeal();
            CombatUnlocked().store(false, std::memory_order_release);
            DieQuiet();
            break;
        }
    }
}

inline void StartProtectThread()
{
#if defined(NDEBUG)
    if (ProtectWatch().exchange(true))
        return;
    ProtectThreadHandle() = std::thread(ProtectThreadMain);
#endif
}

inline void StopProtectThread()
{
#if defined(NDEBUG)
    ProtectWatch().store(false, std::memory_order_release);
    if (ProtectThreadHandle().joinable())
        ProtectThreadHandle().join();
#endif
}

// Call once early in main (Release only)
inline void Bootstrap()
{
#if defined(NDEBUG)
    HardenProcess();
    if (!Harden::Init())
    {
        // Second instance already showed a MessageBox; otherwise fail soft
        // (never silent TerminateProcess on startup — that looked like "instant close")
        return;
    }
    StartProtectThread();
#endif
}

} // namespace Protect
