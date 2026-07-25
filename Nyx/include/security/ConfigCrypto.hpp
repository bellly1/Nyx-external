#pragma once

// =============================================================================
// Layer: Encrypted configuration data at rest
// =============================================================================
// XOR + Base64 with machine-derived key (obfuscation, not military crypto).
// Suitable for local license cache / non-critical config blobs.
// =============================================================================

#include "HwId.hpp"
#include <string>
#include <vector>
#include <cctype>

namespace Security
{

class ConfigCrypto
{
public:
    static std::string Protect(const std::string& plain)
    {
        const std::string key = machineKey();
        std::string x(plain.size(), '\0');
        for (size_t i = 0; i < plain.size(); ++i)
            x[i] = static_cast<char>(static_cast<unsigned char>(plain[i])
                ^ static_cast<unsigned char>(key[i % key.size()]) ^ 0x5A);
        return b64Encode(x);
    }

    static std::string Unprotect(const std::string& b64)
    {
        const std::string x = b64Decode(b64);
        const std::string key = machineKey();
        std::string plain(x.size(), '\0');
        for (size_t i = 0; i < x.size(); ++i)
            plain[i] = static_cast<char>(static_cast<unsigned char>(x[i])
                ^ static_cast<unsigned char>(key[i % key.size()]) ^ 0x5A);
        return plain;
    }

private:
    static std::string machineKey()
    {
        std::string k = HwId::Collect();
        if (k.empty()) k = "nyx-local-key";
        return k;
    }

    static std::string b64Encode(const std::string& in)
    {
        static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string o;
        o.reserve(((in.size() + 2) / 3) * 4);
        size_t i = 0;
        while (i + 2 < in.size())
        {
            const unsigned n = (unsigned char)in[i] << 16 | (unsigned char)in[i + 1] << 8 | (unsigned char)in[i + 2];
            o.push_back(t[(n >> 18) & 63]);
            o.push_back(t[(n >> 12) & 63]);
            o.push_back(t[(n >> 6) & 63]);
            o.push_back(t[n & 63]);
            i += 3;
        }
        if (i < in.size())
        {
            unsigned n = (unsigned char)in[i] << 16;
            o.push_back(t[(n >> 18) & 63]);
            if (i + 1 < in.size())
            {
                n |= (unsigned char)in[i + 1] << 8;
                o.push_back(t[(n >> 12) & 63]);
                o.push_back(t[(n >> 6) & 63]);
                o.push_back('=');
            }
            else
            {
                o.push_back(t[(n >> 12) & 63]);
                o.push_back('=');
                o.push_back('=');
            }
        }
        return o;
    }

    static std::string b64Decode(const std::string& in)
    {
        auto val = [](char c) -> int {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
            if (c >= '0' && c <= '9') return c - '0' + 52;
            if (c == '+') return 62;
            if (c == '/') return 63;
            return -1;
        };
        std::string o;
        int buf = 0, bits = 0;
        for (char c : in)
        {
            if (c == '=' || std::isspace(static_cast<unsigned char>(c))) continue;
            int v = val(c);
            if (v < 0) continue;
            buf = (buf << 6) | v;
            bits += 6;
            if (bits >= 8)
            {
                bits -= 8;
                o.push_back(static_cast<char>((buf >> bits) & 0xFF));
            }
        }
        return o;
    }
};

} // namespace Security
