#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <cstdint>

class Memory
{
public:
    Memory() = default;
    ~Memory();

    bool Attach(const std::wstring& processName);
    void Detach();

    bool IsAttached() const;
    bool IsProcessAlive() const;
    uintptr_t GetModuleBase(const std::wstring& moduleName) const;
    DWORD GetProcessId() const;
    HANDLE Handle() const { return m_handle; }

    template <typename T>
    T Read(uintptr_t address) const
    {
        T value{};
        ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr);
        return value;
    }

    template <typename T>
    bool Write(uintptr_t address, const T& value) const
    {
        return WriteProcessMemory(m_handle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr) != FALSE;
    }

    bool ReadRaw(uintptr_t address, void* buffer, size_t size) const;
    bool WriteRaw(uintptr_t address, const void* buffer, size_t size) const;
    std::string ReadString(uintptr_t address) const;
    std::string ReadStringPointer(uintptr_t address) const;
    bool WriteString(uintptr_t address, const std::string& str) const;

private:
    HANDLE m_handle = nullptr;
    DWORD m_processId = 0;
};

