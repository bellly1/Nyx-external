#pragma once

// =============================================================================
// Layer: Runtime integrity / anti-tamper (critical modules)
// =============================================================================
// Captures dual hashes (CRC32 + FNV) of PE headers + executable section.
// Lock() after successful license. Verify() detects patches (e.g. x64dbg).
// On failure: refuse sensitive ops gracefully — do not crash.
//
// Performance: O(section size) on Lock/Verify. Call Verify at most every few
// seconds from a background thread, never every frame.
// =============================================================================

#include <cstdint>
#include <atomic>

namespace Security
{

class IntegrityMonitor
{
public:
    IntegrityMonitor() = default;

    // Capture baseline of this module (call once after load / after license)
    bool CaptureSelf();

    // Freeze baseline after auth (subsequent Verify uses this snapshot)
    void Lock();

    [[nodiscard]] bool IsLocked() const noexcept { return m_locked.load(std::memory_order_acquire); }
    [[nodiscard]] bool IsReady() const noexcept { return m_ready.load(std::memory_order_acquire); }

    // Compare live image to baseline. true = OK, false = modified
    [[nodiscard]] bool Verify() const;

private:
    static uint32_t Crc32(const void* data, size_t len);
    static uint64_t Fnv64(const void* data, size_t len);

    std::atomic<bool> m_ready{ false };
    std::atomic<bool> m_locked{ false };
    uint32_t m_headerCrc = 0;
    uint32_t m_textCrc = 0;
    uint64_t m_textFnv = 0;
    const uint8_t* m_textBase = nullptr;
    size_t m_textSize = 0;
    uint32_t m_headerSize = 0;
    const uint8_t* m_imageBase = nullptr;
};

} // namespace Security
