#pragma once

// ============================================================================
// Nyx runtime hardening (in-process)
// ============================================================================
// FULL commercial-grade virtualization / mutation / CFF / PE packers are NOT
// reimplemented here — use VMProtect / Themida / Enigma AFTER building Release.
//
// This module covers what we can do safely in source without freezing the menu:
//   • Integrity (image checksum, .text CRC, periodic re-check)
//   • API hashing + dynamic resolve (IAT hiding for selected APIs)
//   • Anti-debug / anti-VM / anti-sandbox (scored, cached)
//   • Hook detection (inline patch on critical imports)
//   • Injection heuristics (unsigned / blacklisted modules)
//   • Single-instance mutex
//   • Encrypted constants, opaque predicates, junk
//   • Watchdog tick integration
//
// See PROTECTION.md for the full checklist vs external tools.
// ============================================================================

#include "skStr.h"
#include "security/AntiTamper.hpp"
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <atomic>
#include <intrin.h>

namespace Harden
{

inline std::atomic<bool>& OkFlag()
{
    static std::atomic<bool> g{ true };
    return g;
}

// ---------------------------------------------------------------------------
// Compile-time / runtime constant encryption
// ---------------------------------------------------------------------------
template <uint64_t Key, typename T>
struct EncConst
{
    T enc;
    constexpr explicit EncConst(T v)
        : enc(static_cast<T>(static_cast<uint64_t>(v) ^ Key)) {}
    __forceinline T get() const
    {
        return static_cast<T>(static_cast<uint64_t>(enc) ^ Key);
    }
};

#define ENC_U32(v) (Harden::EncConst<0xC0FFEEULL ^ __LINE__, uint32_t>(v).get())
#define ENC_U64(v) (Harden::EncConst<0xBADC0DEULL ^ __LINE__, uint64_t>(v).get())

// ---------------------------------------------------------------------------
// Opaque predicates + junk (anti-decompilation noise)
// ---------------------------------------------------------------------------
inline volatile int g_opaque = 0x13579BDF;

__forceinline bool OpaqueTrue()
{
    g_opaque ^= (int)(uintptr_t)&g_opaque;
    const int x = g_opaque | 1;
    return (x * x) > 0;
}

__forceinline bool OpaqueFalse()
{
    g_opaque += 0x1111;
    return (g_opaque & 0) != 0;
}

__forceinline void JunkNoise()
{
    volatile uint64_t a = __rdtsc();
    a ^= a << 13; a ^= a >> 7; a ^= a << 17;
    g_opaque ^= (int)a;
}

// ---------------------------------------------------------------------------
// API hashing (djb2) + dynamic resolve — hides plain import names at call sites
// ---------------------------------------------------------------------------
constexpr uint32_t HashApi(const char* s)
{
    uint32_t h = 5381u;
    while (*s)
        h = ((h << 5) + h) + (uint8_t)*s++;
    return h;
}

inline FARPROC ResolveByHash(HMODULE mod, uint32_t want)
{
    if (!mod) return nullptr;
    auto* base = (uint8_t*)mod;
    auto* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;
    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dir.VirtualAddress) return nullptr;
    auto* exp = (IMAGE_EXPORT_DIRECTORY*)(base + dir.VirtualAddress);
    auto* names = (DWORD*)(base + exp->AddressOfNames);
    auto* ords = (WORD*)(base + exp->AddressOfNameOrdinals);
    auto* funcs = (DWORD*)(base + exp->AddressOfFunctions);
    for (DWORD i = 0; i < exp->NumberOfNames; ++i)
    {
        const char* nm = (const char*)(base + names[i]);
        if (HashApi(nm) == want)
            return (FARPROC)(base + funcs[ords[i]]);
    }
    return nullptr;
}

inline HMODULE Mod(const char* name)
{
    return GetModuleHandleA(name);
}

// Precomputed hashes for common APIs (computed offline — not plaintext strings)
// "IsDebuggerPresent" djb2 — compute at runtime once from encrypted string instead
inline uint32_t H_IsDebuggerPresent()
{
    static uint32_t h = 0;
    if (!h)
    {
        auto s = skCrypt("IsDebuggerPresent");
        h = HashApi(s.decrypt());
        s.encrypt();
    }
    return h;
}
inline uint32_t H_CheckRemoteDebuggerPresent()
{
    static uint32_t h = 0;
    if (!h)
    {
        auto s = skCrypt("CheckRemoteDebuggerPresent");
        h = HashApi(s.decrypt());
        s.encrypt();
    }
    return h;
}
inline uint32_t H_NtQueryInformationProcess()
{
    static uint32_t h = 0;
    if (!h)
    {
        auto s = skCrypt("NtQueryInformationProcess");
        h = HashApi(s.decrypt());
        s.encrypt();
    }
    return h;
}

using Fn_IsDebuggerPresent = BOOL(WINAPI*)();
using Fn_CheckRemote = BOOL(WINAPI*)(HANDLE, PBOOL);
using Fn_NtQIP = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);

inline Fn_IsDebuggerPresent Api_IsDebuggerPresent()
{
    static Fn_IsDebuggerPresent f = nullptr;
    if (!f)
        f = (Fn_IsDebuggerPresent)ResolveByHash(Mod("kernel32.dll"), H_IsDebuggerPresent());
    if (!f) f = ::IsDebuggerPresent;
    return f;
}

// ---------------------------------------------------------------------------
// Image integrity — dual hash (CRC32 + FNV). Catches x64dbg byte patches.
// Baseline is frozen AFTER successful login so crack patches to .text fail.
// ---------------------------------------------------------------------------
struct ImageIntegrity
{
    uint32_t headerCrc = 0;
    uint32_t textCrc = 0;
    uint64_t textFnv = 0;
    uint8_t* textBase = nullptr;
    size_t   textSize = 0;
    bool     ready = false;
    bool     locked = false; // after auth — enforce hard
};

inline ImageIntegrity& Img()
{
    static ImageIntegrity i;
    return i;
}

inline uint32_t Crc32(const void* data, size_t len, uint32_t crc = 0xFFFFFFFFu)
{
    const auto* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= p[i];
        for (int b = 0; b < 8; ++b)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

inline uint64_t FnvText(const void* data, size_t len)
{
    const auto* p = (const uint8_t*)data;
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < len; ++i)
    {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

inline bool CaptureImageIntegrity()
{
    HMODULE mod = GetModuleHandleW(nullptr);
    if (!mod) return false;
    auto* base = (uint8_t*)mod;
    auto* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

    auto& img = Img();
    const DWORD hdrSize = nt->OptionalHeader.SizeOfHeaders;
    img.headerCrc = Crc32(base, hdrSize);

    auto* sec = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    {
        if (sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)
        {
            img.textBase = base + sec[i].VirtualAddress;
            img.textSize = sec[i].Misc.VirtualSize;
            // Larger window so more of the binary is covered (still bounded for FPS)
            if (img.textSize > 1024 * 1024)
                img.textSize = 1024 * 1024;
            img.textCrc = Crc32(img.textBase, img.textSize);
            img.textFnv = FnvText(img.textBase, img.textSize);
            img.ready = true;
            return true;
        }
    }
    return false;
}

// Call once right after successful auth — freezes “clean” bytes
inline void LockIntegrityBaseline()
{
    CaptureImageIntegrity();
    Img().locked = true;
}

inline bool VerifyImageIntegrity()
{
    auto& img = Img();
    if (!img.ready) return true;
    HMODULE mod = GetModuleHandleW(nullptr);
    if (!mod) return false;
    auto* base = (uint8_t*)mod;
    auto* dos = (IMAGE_DOS_HEADER*)base;
    auto* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    const DWORD hdrSize = nt->OptionalHeader.SizeOfHeaders;
    if (Crc32(base, hdrSize) != img.headerCrc)
        return false;
    if (img.textBase && img.textSize)
    {
        if (Crc32(img.textBase, img.textSize) != img.textCrc)
            return false;
        if (FnvText(img.textBase, img.textSize) != img.textFnv)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Debugger / x64dbg detection (the usual “patch in debug64” workflow)
// ---------------------------------------------------------------------------
inline bool PebBeingDebugged()
{
    // Only BeingDebugged flag — NtGlobalFlag heap bits false-positive often
#ifdef _WIN64
    const auto* peb = (const unsigned char*)__readgsqword(0x60);
    return peb && peb[2] != 0;
#else
    const auto* peb = (const unsigned char*)__readfsdword(0x30);
    return peb && peb[2] != 0;
#endif
}

inline bool DebuggerAttachedNow()
{
    auto idp = Api_IsDebuggerPresent();
    if (idp && idp()) return true;
    BOOL remote = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote)
        return true;
    if (PebBeingDebugged()) return true;
    return false;
}

// Delegates to Security::AntiTamper (detect + optional kill)
inline bool MatchCrackerName(const char* nameLower)
{
    return Security::AntiTamper::IsBlacklistedProcessName(nameLower);
}

inline bool CrackerToolPresent()
{
    return Security::AntiTamper::CrackerProcessPresent();
}

// Kill blacklisted RE tools (x64dbg, CE, IDA, Scylla, …). Returns kill attempts.
// Will increase AV detections — intentional max-protection policy.
inline int KillCrackerTools()
{
#if defined(NDEBUG)
    return Security::AntiTamper::NeutralizeCrackerProcesses();
#else
    return 0;
#endif
}

// Safe anti-dump — DO NOT wipe PE headers (that crashed the process on launch)
inline void ApplyAntiDump()
{
#if defined(NDEBUG)
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
#endif
}

// Only kill for real debugger attach or explicit OkFlag death — not noisy scans at startup
inline void KillIfHostile()
{
#if defined(NDEBUG)
    if (DebuggerAttachedNow() || !OkFlag().load(std::memory_order_relaxed))
    {
        OkFlag().store(false);
        TerminateProcess(GetCurrentProcess(), 0xDEAD);
    }
#endif
}

// Markers for VMProtect (no-op without SDK). After packing, map these ranges in VMP.
#ifndef VMP_BEGIN
#define VMP_BEGIN(name) ((void)0)
#define VMP_END()       ((void)0)
#endif

// Capability token — features must re-derive this; patching one `bool success` is not enough
inline std::atomic<uint64_t>& Capability()
{
    static std::atomic<uint64_t> c{ 0 };
    return c;
}

inline void BindCapability(const char* sessionOrToken, const char* hwid)
{
    uint64_t h = 14695981039346656037ull;
    auto mix = [&](const char* s) {
        if (!s) return;
        for (; *s; ++s) { h ^= (uint8_t)*s; h *= 1099511628211ull; }
    };
    mix(sessionOrToken);
    mix("|");
    mix(hwid);
    mix("|");
    h ^= GetCurrentProcessId() * 0x9E3779B97F4A7C15ull;
    h ^= 0x0B51D1AFull;
    Capability().store(h, std::memory_order_release);
}

// True if BindCapability ran after a real login (do not re-hash Seal buffers —
// JWT tokens are longer than the 96-char seal storage and always mismatched).
inline bool CapabilityOk(const char* /*sessionOrToken*/, const char* /*hwid*/)
{
    return Capability().load(std::memory_order_acquire) != 0;
}

// Single gate used from aim / overlay / movement — hard to NOP once
inline bool RuntimeClean()
{
#if !defined(NDEBUG)
    return true;
#else
    // Soft check for hot paths — integrity is enforced on background thread
    // (hard-failing CRC here closed the app right after attach)
    if (DebuggerAttachedNow())
        return false;
    if (!OkFlag().load(std::memory_order_relaxed))
        return false;
    return true;
#endif
}

// ---------------------------------------------------------------------------
// Software breakpoint / inline hook detection on a few critical stubs
// ---------------------------------------------------------------------------
inline bool PageLooksHooked(const void* fn)
{
    if (!fn) return false;
    const auto* p = (const uint8_t*)fn;
    // Common hooks: JMP rel32 (E9), JMP [rip] (FF 25), INT3 (CC), MOV RAX; JMP RAX
    if (p[0] == 0xE9 || p[0] == 0xCC || p[0] == 0xC3)
        return p[0] == 0xE9 || p[0] == 0xCC;
    if (p[0] == 0xFF && (p[1] == 0x25 || p[1] == 0x15))
        return true;
    if (p[0] == 0x48 && p[1] == 0xB8) // mov rax, imm64 — often trampoline
        return true;
    return false;
}

inline bool CriticalApisHooked()
{
    HMODULE k32 = Mod("kernel32.dll");
    HMODULE ntdll = Mod("ntdll.dll");
    if (!k32) return false;

    auto isDbg = ResolveByHash(k32, H_IsDebuggerPresent());
    auto chkRem = ResolveByHash(k32, H_CheckRemoteDebuggerPresent());
    auto ntq = ntdll ? ResolveByHash(ntdll, H_NtQueryInformationProcess()) : nullptr;

    if (PageLooksHooked(isDbg)) return true;
    if (PageLooksHooked(chkRem)) return true;
    if (PageLooksHooked(ntq)) return true;
    if (PageLooksHooked((void*)&VirtualProtect)) return true;
    if (PageLooksHooked((void*)&ReadProcessMemory)) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Anti-VM / sandbox (scored — soft; does not alone kill)
// ---------------------------------------------------------------------------
inline bool CpuIdHypervisor()
{
    int cpu[4]{};
    __cpuid(cpu, 1);
    return (cpu[2] & (1 << 31)) != 0; // hypervisor bit
}

inline bool VmArtifacts()
{
    // Registry keys commonly present in VMs
    const wchar_t* keys[] = {
        L"SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
        L"SYSTEM\\CurrentControlSet\\Services\\VBoxMouse",
        L"SYSTEM\\CurrentControlSet\\Services\\vmci",
        L"SYSTEM\\CurrentControlSet\\Services\\vmhgfs",
        L"SOFTWARE\\VMware, Inc.\\VMware Tools",
        L"SOFTWARE\\Oracle\\VirtualBox Guest Additions",
    };
    for (auto* k : keys)
    {
        HKEY h = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, k, 0, KEY_READ, &h) == ERROR_SUCCESS)
        {
            RegCloseKey(h);
            return true;
        }
    }
    // MAC OUI prefixes (VMware / VBox) — best-effort via GetAdapters not linked; skip heavy
    return false;
}

inline bool SandboxUser()
{
    char user[256]{};
    DWORD n = 256;
    if (!GetUserNameA(user, &n)) return false;
    for (char* p = user; *p; ++p)
        *p = (char)tolower((unsigned char)*p);
    // common sandbox usernames
    return std::strstr(user, "sandbox") || std::strstr(user, "virus")
        || std::strstr(user, "malware") || std::strstr(user, "currentuser")
        || std::strstr(user, "john") && GetTickCount64() < 10 * 60 * 1000; // weak
}

// ---------------------------------------------------------------------------
// Injection heuristics — unexpected modules
// ---------------------------------------------------------------------------
inline bool SuspiciousInjectedModule()
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    bool hit = false;
    if (Module32FirstW(snap, &me))
    {
        do
        {
            char name[MAX_PATH]{};
            WideCharToMultiByte(CP_UTF8, 0, me.szModule, -1, name, MAX_PATH, nullptr, nullptr);
            for (char* p = name; *p; ++p)
                *p = (char)tolower((unsigned char)*p);

            if (std::strstr(name, "inject") || std::strstr(name, "vehdebug")
                || std::strstr(name, "scyllahide") || std::strstr(name, "sharpod")
                || std::strstr(name, "hook") && std::strstr(name, "dll")
                || std::strstr(name, "sbiedll") // sandboxie
                || std::strstr(name, "cmdvrt"))
            {
                hit = true;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return hit;
}

// ---------------------------------------------------------------------------
// Hardware breakpoints (DR0-DR3)
// ---------------------------------------------------------------------------
inline bool HardwareBreakpointsSet()
{
    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(GetCurrentThread(), &ctx))
        return false;
    return (ctx.Dr0 | ctx.Dr1 | ctx.Dr2 | ctx.Dr3) != 0;
}

// ---------------------------------------------------------------------------
// Single-instance mutex
// ---------------------------------------------------------------------------
inline HANDLE g_instanceMutex = nullptr;

inline bool EnforceSingleInstance()
{
    g_instanceMutex = CreateMutexW(nullptr, TRUE, L"Local\\NyxRuntime_v1");
    if (!g_instanceMutex) return true;
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return false;
    return true;
}

// ---------------------------------------------------------------------------
// Timing anti-debug (soft)
// ---------------------------------------------------------------------------
inline bool TimingAnomaly()
{
    const uint64_t t0 = __rdtsc();
    volatile int x = 0;
    for (int i = 0; i < 500; ++i) x += i;
    const uint64_t dt = __rdtsc() - t0;
    (void)x;
    return dt > 50ull * 1000ull * 1000ull; // very high = single-step
}

// ---------------------------------------------------------------------------
// Aggregated threat score (cached)
// ---------------------------------------------------------------------------
struct ThreatState
{
    std::atomic<int> score{ 0 };
    std::atomic<uint32_t> flags{ 0 };
    std::atomic<ULONGLONG> lastMs{ 0 };
};

inline ThreatState& Threat()
{
    static ThreatState t;
    return t;
}

enum Flag : uint32_t
{
    F_Debugger     = 1u << 0,
    F_Integrity    = 1u << 1,
    F_Hook         = 1u << 2,
    F_Inject       = 1u << 3,
    F_VM           = 1u << 4,
    F_HWBP         = 1u << 5,
    F_Timing       = 1u << 6,
    F_Sandbox      = 1u << 7,
};

inline int RescanThreats()
{
    int score = 0;
    uint32_t flags = 0;

    auto idp = Api_IsDebuggerPresent();
    if (idp && idp())
    {
        score += 100;
        flags |= F_Debugger;
    }
    BOOL remote = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote)
    {
        score += 100;
        flags |= F_Debugger;
    }
    if (HardwareBreakpointsSet())
    {
        score += 60;
        flags |= F_HWBP;
    }
    if (!VerifyImageIntegrity())
    {
        score += 100;
        flags |= F_Integrity;
    }
    if (CriticalApisHooked())
    {
        score += 80;
        flags |= F_Hook;
    }
    if (SuspiciousInjectedModule())
    {
        score += 70;
        flags |= F_Inject;
    }
    if (CpuIdHypervisor() && VmArtifacts())
    {
        score += 40; // soft — many gamers use Hyper-V
        flags |= F_VM;
    }
    if (SandboxUser())
    {
        score += 30;
        flags |= F_Sandbox;
    }
    if (TimingAnomaly())
    {
        score += 20;
        flags |= F_Timing;
    }

    Threat().score.store(score, std::memory_order_relaxed);
    Threat().flags.store(flags, std::memory_order_relaxed);
    Threat().lastMs.store(GetTickCount64(), std::memory_order_relaxed);
    return score;
}

// Call from background thread every few seconds
inline int ThreatScoreCached(ULONGLONG minIntervalMs = 4000)
{
    const ULONGLONG now = GetTickCount64();
    if (now - Threat().lastMs.load(std::memory_order_relaxed) < minIntervalMs)
        return Threat().score.load(std::memory_order_relaxed);
    return RescanThreats();
}

// Hard kill threshold — integrity / debugger only (not VM alone)
inline bool ShouldTripFatal()
{
    const int s = ThreatScoreCached();
    const uint32_t f = Threat().flags.load(std::memory_order_relaxed);
    if (f & (F_Debugger | F_Integrity))
        return true;
    if (s >= 150 && (f & (F_Hook | F_Inject)))
        return true;
    return false;
}

// ---------------------------------------------------------------------------
// Mini “virtualization” of a seal check — stack VM (selected logic only)
// Not a full x86 VMProtect replacement — raises reverse-engineering cost slightly
// ---------------------------------------------------------------------------
enum VOp : uint8_t
{
    V_PUSH = 1, V_ADD, V_XOR, V_EQ, V_RET, V_NOP = 0x90
};

inline bool VmEvalSealFragment(uint64_t a, uint64_t b, uint64_t expectXor)
{
    // Program: push a; push b; xor; push expect; eq; ret
    const uint8_t prog[] = {
        V_PUSH, 0, // slot a
        V_PUSH, 1, // slot b
        V_XOR,
        V_PUSH, 2, // expect
        V_EQ,
        V_RET
    };
    uint64_t slots[3] = { a, b, expectXor };
    uint64_t stack[8]{};
    int sp = 0;
    for (size_t ip = 0; ip < sizeof(prog); )
    {
        switch (prog[ip++])
        {
        case V_NOP: break;
        case V_PUSH:
            if (ip >= sizeof(prog) || sp >= 8) return false;
            stack[sp++] = slots[prog[ip++]];
            break;
        case V_ADD:
            if (sp < 2) return false;
            stack[sp - 2] = stack[sp - 2] + stack[sp - 1];
            --sp;
            break;
        case V_XOR:
            if (sp < 2) return false;
            stack[sp - 2] = stack[sp - 2] ^ stack[sp - 1];
            --sp;
            break;
        case V_EQ:
            if (sp < 2) return false;
            stack[sp - 2] = (stack[sp - 2] == stack[sp - 1]) ? 1 : 0;
            --sp;
            break;
        case V_RET:
            return sp >= 1 && stack[sp - 1] != 0;
        default:
            return false;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Secure alloc helper (page guard optional)
// ---------------------------------------------------------------------------
inline void* SecureAlloc(size_t n)
{
    void* p = VirtualAlloc(nullptr, n, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    return p;
}

inline void SecureFree(void* p, size_t n)
{
    if (!p) return;
    SecureZeroMemory(p, n);
    VirtualFree(p, 0, MEM_RELEASE);
}

// ---------------------------------------------------------------------------
// Init / tick
// ---------------------------------------------------------------------------
inline bool Init()
{
#if !defined(NDEBUG)
    return true;
#else
    if (!EnforceSingleInstance())
    {
        MessageBoxW(nullptr, L"Already running.", L"Nyx", MB_OK | MB_ICONWARNING);
        return false;
    }

    // Safe init only — PE wipe / tool scan here caused instant close
    CaptureImageIntegrity();
    SetProcessDEPPolicy(PROCESS_DEP_ENABLE);

    if (DebuggerAttachedNow())
    {
        OkFlag().store(false);
        return false;
    }
    return true;
#endif
}

// Background — call every few seconds from protect thread
inline void Tick()
{
#if defined(NDEBUG)
    JunkNoise();

    if (DebuggerAttachedNow())
    {
        OkFlag().store(false);
        return;
    }

    // Detect + kill RE tools (TerminateProcess on blacklist)
    if (KillCrackerTools() > 0)
    {
        OkFlag().store(false);
        return;
    }

    if (Img().locked && !VerifyImageIntegrity())
    {
        OkFlag().store(false);
        return;
    }
#endif
}

inline bool Alive()
{
    return OkFlag().load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Build-time / packer notes (not code — for PROTECTION.md)
// Virtualization, mutation, CFF, full IAT wipe, PE packing → VMProtect post-build
// /GUARD:CF, /HIGHENTROPYVA, /NXCOMPAT → already MSVC linker flags
// Strip PDB: Project → Release → Generate Debug Info = No
// Code sign: signtool after pack
// ---------------------------------------------------------------------------

} // namespace Harden
