#include "Memory.hpp"

#include <Psapi.h>
#include <cstring>
#pragma comment(lib, "Psapi.lib")

Memory::~Memory()
{
    Detach();
}

bool Memory::Attach(const std::wstring& processName)
{
    Detach();

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0)
            {
                m_processId = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    if (!m_processId)
        return false;

    m_handle = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE,
        m_processId);
    if (!m_handle)
    {
        m_handle = OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_LIMITED_INFORMATION,
            FALSE,
            m_processId);
    }
    return m_handle != nullptr;
}

void Memory::Detach()
{
    if (m_handle)
    {
        CloseHandle(m_handle);
        m_handle = nullptr;
    }
    m_processId = 0;
}

bool Memory::IsAttached() const
{
    return m_handle != nullptr;
}

bool Memory::IsProcessAlive() const
{
    if (!m_handle || !m_processId)
        return false;

    DWORD code = 0;
    if (!GetExitCodeProcess(m_handle, &code))
        return false;
    // Fast path only — process snapshot every frame kills FPS
    return code == STILL_ACTIVE;
}

DWORD Memory::GetProcessId() const
{
    return m_processId;
}

uintptr_t Memory::GetModuleBase(const std::wstring& moduleName) const
{
    if (!m_handle || !m_processId)
        return 0;

    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_processId);
        if (snapshot != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            uintptr_t base = 0;
            if (Module32FirstW(snapshot, &entry))
            {
                do
                {
                    if (_wcsicmp(entry.szModule, moduleName.c_str()) == 0)
                    {
                        base = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                        break;
                    }
                } while (Module32NextW(snapshot, &entry));
            }
            CloseHandle(snapshot);
            if (base) return base;
        }
    }

    {
        HMODULE mods[1024]{};
        DWORD needed = 0;
        if (EnumProcessModulesEx(m_handle, mods, sizeof(mods), &needed, LIST_MODULES_ALL))
        {
            const unsigned count = needed / sizeof(HMODULE);
            for (unsigned i = 0; i < count && i < 1024; ++i)
            {
                wchar_t name[MAX_PATH]{};
                if (GetModuleBaseNameW(m_handle, mods[i], name, MAX_PATH) == 0)
                    continue;
                if (_wcsicmp(name, moduleName.c_str()) == 0)
                    return reinterpret_cast<uintptr_t>(mods[i]);
            }

            if (count > 0 && mods[0])
            {
                wchar_t name[MAX_PATH]{};
                if (GetModuleBaseNameW(m_handle, mods[0], name, MAX_PATH)
                    && _wcsicmp(name, moduleName.c_str()) == 0)
                    return reinterpret_cast<uintptr_t>(mods[0]);

                if (_wcsicmp(moduleName.c_str(), L"RobloxPlayerBeta.exe") == 0)
                    return reinterpret_cast<uintptr_t>(mods[0]);
            }
        }
    }

    return 0;
}

bool Memory::ReadRaw(uintptr_t address, void* buffer, size_t size) const
{
    if (!m_handle || !address || !buffer || !size)
        return false;

    SIZE_T bytesRead = 0;
    return ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(address), buffer, size, &bytesRead)
        && bytesRead == size;
}

bool Memory::WriteRaw(uintptr_t address, const void* buffer, size_t size) const
{
    if (!m_handle || !address || !buffer || !size)
        return false;

    SIZE_T written = 0;
    return WriteProcessMemory(m_handle, reinterpret_cast<LPVOID>(address), buffer, size, &written)
        && written == size;
}

std::string Memory::ReadString(uintptr_t address) const
{
    if (!address)
        return {};

    const int length = Read<int>(address + 0x10);
    if (length <= 0 || length > 1024)
        return {};

    if (length >= 16)
    {
        const uintptr_t ptr = Read<uintptr_t>(address);
        if (!ptr) return {};
        std::string s(static_cast<size_t>(length), '\0');
        if (!ReadRaw(ptr, s.data(), static_cast<size_t>(length)))
            return {};
        return s;
    }

    std::string s(static_cast<size_t>(length), '\0');
    if (!ReadRaw(address, s.data(), static_cast<size_t>(length)))
        return {};
    return s;
}

std::string Memory::ReadStringPointer(uintptr_t address) const
{
    if (!address) return {};
    const uintptr_t ptr = Read<uintptr_t>(address);
    if (!ptr) return {};
    return ReadString(ptr);
}

bool Memory::WriteString(uintptr_t address, const std::string& str) const
{
    if (!m_handle || !address || str.data() == nullptr) return false;

    struct RbxString {
        union {
            char buffer[16];
            uintptr_t pointer;
        } data;
        uintptr_t length;
        uintptr_t capacity;
    };

    RbxString s{};
    if (!ReadRaw(address, &s, sizeof(RbxString)))
        return false;

    if (s.capacity == 0)
        s.capacity = 15;

    constexpr uintptr_t kMaxCapacity = 1u << 20;
    if (s.capacity > kMaxCapacity)
        return false;

    s.length = static_cast<uintptr_t>(str.length());

    if (s.length > 15)
    {
        if (s.length > s.capacity || s.data.pointer <= 0x10000 || s.data.pointer > 0x00007FFFFFFFFFFF)
        {
            s.capacity = s.length + 1;
            if (s.capacity > kMaxCapacity)
                return false;

            LPVOID newBuf = VirtualAllocEx(m_handle, nullptr, static_cast<SIZE_T>(s.capacity),
                                           MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!newBuf) return false;

            s.data.pointer = reinterpret_cast<uintptr_t>(newBuf);
        }

        if (!WriteRaw(address, &s, sizeof(RbxString)))
            return false;
        if (!str.empty() && !WriteRaw(s.data.pointer, str.data(), s.length))
            return false;
        if (!Write<char>(s.data.pointer + s.length, '\0'))
            return false;
    }
    else
    {
        s.capacity = 15;
        std::memset(s.data.buffer, 0, sizeof(s.data.buffer));
        if (!str.empty())
            std::memcpy(s.data.buffer, str.data(), s.length);
        if (s.length < sizeof(s.data.buffer))
            s.data.buffer[s.length] = '\0';

        if (!WriteRaw(address, &s, sizeof(RbxString)))
            return false;
    }

    return true;
}

