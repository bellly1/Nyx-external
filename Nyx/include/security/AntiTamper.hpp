#pragma once

// =============================================================================
// Layer: Anti-tamper + RE-tool neutralization
// =============================================================================
// Detects reverse-engineering / dumping / patching tools.
// Policy (user requested max protection, AV risk accepted):
//   1) Terminate blacklisted processes when found
//   2) Mark session compromised → sensitive ops refuse
//   3) Never run this every frame — only from background (~2–3s)
//
// WARNING: Terminating third-party processes will often flag antivirus.
// That is expected with this policy.
// =============================================================================

#include <Windows.h>
#include <TlHelp32.h>
#include <atomic>
#include <cstring>

namespace Security
{

class AntiTamper
{
public:
    [[nodiscard]] static bool LooksHooked(const void* fn) noexcept
    {
        if (!fn) return false;
        const auto* p = static_cast<const uint8_t*>(fn);
        if (p[0] == 0xE9 || p[0] == 0xCC) return true;
        if (p[0] == 0xFF && (p[1] == 0x25 || p[1] == 0x15)) return true;
        if (p[0] == 0x48 && p[1] == 0xB8) return true;
        return false;
    }

    [[nodiscard]] static bool DebuggerPresentLight() noexcept
    {
        if (IsDebuggerPresent()) return true;
        BOOL remote = FALSE;
        if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote)
            return true;
        return false;
    }

    // Blacklist: RE / dump / patch / common analysis tools (name substrings, lowercased)
    [[nodiscard]] static bool IsBlacklistedProcessName(const char* nameLower) noexcept
    {
        if (!nameLower || !nameLower[0]) return false;

        // Debuggers
        if (std::strstr(nameLower, "x64dbg") || std::strstr(nameLower, "x32dbg")
            || std::strstr(nameLower, "x96dbg") || std::strstr(nameLower, "ollydbg")
            || std::strstr(nameLower, "windbg") || std::strstr(nameLower, "immunitydebugger"))
            return true;

        // Disassemblers / decompilers
        if (std::strstr(nameLower, "ida64") || std::strstr(nameLower, "ida.exe")
            || std::strstr(nameLower, "idaq64") || std::strstr(nameLower, "idaq.exe")
            || std::strstr(nameLower, "idag") || std::strstr(nameLower, "ghidra")
            || std::strstr(nameLower, "binaryninja") || std::strstr(nameLower, "radare2")
            || std::strstr(nameLower, "cutter.exe") || std::strstr(nameLower, "hopper"))
            return true;

        // .NET reverse
        if (std::strstr(nameLower, "dnspy") || std::strstr(nameLower, "ilspy")
            || std::strstr(nameLower, "dotpeek") || std::strstr(nameLower, "de4dot"))
            return true;

        // Dumpers / import rebuilders
        if (std::strstr(nameLower, "scylla") || std::strstr(nameLower, "importrec")
            || std::strstr(nameLower, "megadumper") || std::strstr(nameLower, "extremedumper")
            || std::strstr(nameLower, "x64netdumper") || std::strstr(nameLower, "hollowshunter")
            || std::strstr(nameLower, "pe-sieve") || std::strstr(nameLower, "pesieve")
            || std::strstr(nameLower, "lordpe") || std::strstr(nameLower, "pe-bear")
            || std::strstr(nameLower, "pestudio"))
            return true;

        // Memory editors / trainers commonly used to crack
        if (std::strstr(nameLower, "cheatengine") || std::strstr(nameLower, "artmoney")
            || std::strstr(nameLower, "reclass"))
            return true;

        // HTTP debug (license MITM)
        if (std::strstr(nameLower, "httpdebugger") || std::strstr(nameLower, "fiddler")
            || std::strstr(nameLower, "charles.exe") || std::strstr(nameLower, "mitmproxy")
            || std::strstr(nameLower, "wireshark") || std::strstr(nameLower, "rawcap"))
            return true;

        // Hide / anti-anti-debug helpers often used with x64dbg
        if (std::strstr(nameLower, "scyllahide") || std::strstr(nameLower, "titanhide")
            || std::strstr(nameLower, "sharpod") || std::strstr(nameLower, "vehdebug"))
            return true;

        return false;
    }

    // Detect only (no kill)
    [[nodiscard]] static bool CrackerProcessPresent()
    {
        return ScanAndAct(false) > 0;
    }

    // Detect + TerminateProcess on blacklisted tools. Returns how many kills attempted.
    // Does NOT kill our own PID. May require admin for some elevated tools.
    static int NeutralizeCrackerProcesses()
    {
        return ScanAndAct(true);
    }

    // Full policy: kill tools + mark compromised if any found
    // Returns true if hostile environment was found
    bool enforceProtection()
    {
        if (DebuggerPresentLight())
        {
            markCompromised();
            return true;
        }
        const int n = NeutralizeCrackerProcesses();
        if (n > 0)
        {
            markCompromised();
            return true;
        }
        return false;
    }

    void markCompromised() noexcept { m_compromised.store(true, std::memory_order_release); }
    [[nodiscard]] bool isCompromised() const noexcept
    {
        return m_compromised.load(std::memory_order_acquire);
    }
    void reset() noexcept { m_compromised.store(false, std::memory_order_release); }

private:
    static int ScanAndAct(bool kill)
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;

        const DWORD self = GetCurrentProcessId();
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        int actions = 0;

        if (Process32FirstW(snap, &pe))
        {
            do
            {
                if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4 || pe.th32ProcessID == self)
                    continue;

                char name[MAX_PATH]{};
                WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, name, MAX_PATH, nullptr, nullptr);
                for (char* p = name; *p; ++p)
                    *p = static_cast<char>(::tolower(static_cast<unsigned char>(*p)));

                if (!IsBlacklistedProcessName(name))
                    continue;

                ++actions; // counted as "found" even if kill fails
                if (!kill)
                    continue;

                HANDLE h = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION,
                    FALSE, pe.th32ProcessID);
                if (!h)
                {
                    // Retry with broader rights if possible
                    h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                }
                if (h)
                {
                    TerminateProcess(h, 1);
                    CloseHandle(h);
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return actions;
    }

    std::atomic<bool> m_compromised{ false };
};

} // namespace Security
