#include "security/IntegrityMonitor.hpp"

#include <Windows.h>
#include <algorithm>

namespace Security
{

uint32_t IntegrityMonitor::Crc32(const void* data, size_t len)
{
    auto* p = static_cast<const uint8_t*>(data);
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= p[i];
        for (int b = 0; b < 8; ++b)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

uint64_t IntegrityMonitor::Fnv64(const void* data, size_t len)
{
    auto* p = static_cast<const uint8_t*>(data);
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < len; ++i)
    {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

bool IntegrityMonitor::CaptureSelf()
{
    HMODULE mod = GetModuleHandleW(nullptr);
    if (!mod) return false;
    auto* base = reinterpret_cast<uint8_t*>(mod);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

    m_imageBase = base;
    m_headerSize = nt->OptionalHeader.SizeOfHeaders;
    m_headerCrc = Crc32(base, m_headerSize);

    auto* sec = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    {
        if (sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)
        {
            m_textBase = base + sec[i].VirtualAddress;
            m_textSize = sec[i].Misc.VirtualSize;
            // Cap work for performance (1 MiB)
            m_textSize = (std::min)(m_textSize, size_t{ 1024 * 1024 });
            m_textCrc = Crc32(m_textBase, m_textSize);
            m_textFnv = Fnv64(m_textBase, m_textSize);
            m_ready.store(true, std::memory_order_release);
            return true;
        }
    }
    return false;
}

void IntegrityMonitor::Lock()
{
    CaptureSelf();
    m_locked.store(true, std::memory_order_release);
}

bool IntegrityMonitor::Verify() const
{
    if (!m_ready.load(std::memory_order_acquire) || !m_imageBase)
        return true; // not armed — do not fail closed before Capture

    if (Crc32(m_imageBase, m_headerSize) != m_headerCrc)
        return false;
    if (m_textBase && m_textSize)
    {
        if (Crc32(m_textBase, m_textSize) != m_textCrc)
            return false;
        if (Fnv64(m_textBase, m_textSize) != m_textFnv)
            return false;
    }
    return true;
}

} // namespace Security
