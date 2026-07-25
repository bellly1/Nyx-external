#pragma once

// =============================================================================
// Layer: Minimize sensitive data exposure in memory (RAII wipe)
// =============================================================================
// SecureBuffer holds secrets (tokens, keys) and wipes on destruction / move.
// Prefer this over std::string for session material when practical.
// =============================================================================

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <utility>
#include <string>

namespace Security
{

class SecureBuffer
{
public:
    SecureBuffer() = default;

    explicit SecureBuffer(std::string data)
        : m_data(std::move(data))
    {
    }

    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;

    SecureBuffer(SecureBuffer&& o) noexcept
        : m_data(std::move(o.m_data))
    {
        o.wipe();
    }

    SecureBuffer& operator=(SecureBuffer&& o) noexcept
    {
        if (this != &o)
        {
            wipe();
            m_data = std::move(o.m_data);
            o.wipe();
        }
        return *this;
    }

    ~SecureBuffer()
    {
        wipe();
    }

    void assign(std::string v)
    {
        wipe();
        m_data = std::move(v);
    }

    void wipe()
    {
        if (!m_data.empty())
        {
            SecureZeroMemory(m_data.data(), m_data.size());
            m_data.clear();
            m_data.shrink_to_fit();
        }
    }

    [[nodiscard]] bool empty() const noexcept { return m_data.empty(); }
    [[nodiscard]] size_t size() const noexcept { return m_data.size(); }
    [[nodiscard]] const std::string& view() const noexcept { return m_data; }
    [[nodiscard]] const char* c_str() const noexcept { return m_data.c_str(); }

private:
    std::string m_data;
};

// Compile-time XOR of integer constants (not cryptographically strong — raises static scan cost)
template <uint64_t Key, typename T>
struct EncConst
{
    T enc{};
    constexpr explicit EncConst(T v) noexcept
        : enc(static_cast<T>(static_cast<uint64_t>(v) ^ Key))
    {
    }
    [[nodiscard]] constexpr T get() const noexcept
    {
        return static_cast<T>(static_cast<uint64_t>(enc) ^ Key);
    }
};

#define SEC_ENC_U32(v) (::Security::EncConst<0xA5C3F191ull ^ __LINE__, uint32_t>(static_cast<uint32_t>(v)).get())

} // namespace Security
