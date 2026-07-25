#pragma once

// =============================================================================
// Layer: Secure session / token handling
// =============================================================================
// Holds server-issued session material. Client never "decides" license alone —
// token must come from HTTPS server response. Wiped on clear/destruct.
// =============================================================================

#include "SecureBuffer.hpp"
#include <atomic>
#include <cstdint>
#include <string>

namespace Security
{

class SessionToken
{
public:
    void setFromServer(std::string token)
    {
        m_token.assign(std::move(token));
        m_bound.store(!m_token.empty(), std::memory_order_release);
        recomputeCapability();
    }

    void setHwid(std::string hwid)
    {
        m_hwid.assign(std::move(hwid));
        recomputeCapability();
    }

    void clear()
    {
        m_token.wipe();
        m_hwid.wipe();
        m_capability.store(0, std::memory_order_release);
        m_bound.store(false, std::memory_order_release);
    }

    [[nodiscard]] bool isBound() const noexcept
    {
        return m_bound.load(std::memory_order_acquire) && m_capability.load() != 0;
    }

    [[nodiscard]] bool tokenLooksReal() const
    {
        const auto& t = m_token.view();
        if (t.size() < 16) return false;
        if (t == "null" || t == "undefined" || t == "test" || t == "true")
            return false;
        return true;
    }

    [[nodiscard]] uint64_t capability() const noexcept
    {
        return m_capability.load(std::memory_order_acquire);
    }

    [[nodiscard]] const SecureBuffer& token() const noexcept { return m_token; }

private:
    void recomputeCapability()
    {
        // Hash only — never store full JWT in a tiny fixed seal buffer for compare
        uint64_t h = 14695981039346656037ull;
        auto mix = [&](const std::string& s) {
            for (unsigned char c : s)
            {
                h ^= c;
                h *= 1099511628211ull;
            }
        };
        mix(m_token.view());
        h ^= static_cast<uint64_t>('|');
        mix(m_hwid.view());
        h ^= 0x0B51D1AFull;
        if (m_token.empty())
            h = 0;
        m_capability.store(h, std::memory_order_release);
    }

    SecureBuffer m_token;
    SecureBuffer m_hwid;
    std::atomic<uint64_t> m_capability{ 0 };
    std::atomic<bool> m_bound{ false };
};

} // namespace Security
