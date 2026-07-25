#pragma once

// =============================================================================
// Layer: Import / API hiding (selected APIs only)
// =============================================================================
// Resolves exports by hash so plaintext names need not appear at every call site.
// Keep usage limited to security-sensitive APIs for maintainability.
// =============================================================================

#include <Windows.h>
#include <cstdint>

namespace Security
{

[[nodiscard]] constexpr uint32_t HashApiDjb2(const char* s) noexcept
{
    uint32_t h = 5381u;
    while (s && *s)
        h = ((h << 5) + h) + static_cast<uint8_t>(*s++);
    return h;
}

inline FARPROC ResolveExportByHash(HMODULE mod, uint32_t want) noexcept
{
    if (!mod) return nullptr;
    auto* base = reinterpret_cast<uint8_t*>(mod);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;
    const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dir.VirtualAddress) return nullptr;
    auto* exp = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base + dir.VirtualAddress);
    auto* names = reinterpret_cast<DWORD*>(base + exp->AddressOfNames);
    auto* ords = reinterpret_cast<WORD*>(base + exp->AddressOfNameOrdinals);
    auto* funcs = reinterpret_cast<DWORD*>(base + exp->AddressOfFunctions);
    for (DWORD i = 0; i < exp->NumberOfNames; ++i)
    {
        const char* nm = reinterpret_cast<const char*>(base + names[i]);
        if (HashApiDjb2(nm) == want)
            return reinterpret_cast<FARPROC>(base + funcs[ords[i]]);
    }
    return nullptr;
}

template <typename Fn>
inline Fn GetApi(HMODULE mod, uint32_t hash, Fn fallback) noexcept
{
    auto p = reinterpret_cast<Fn>(ResolveExportByHash(mod, hash));
    return p ? p : fallback;
}

} // namespace Security
