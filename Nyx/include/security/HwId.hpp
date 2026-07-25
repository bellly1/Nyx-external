#pragma once

// =============================================================================
// Layer: Hardware ID collection (license binding)
// =============================================================================
// Uses standard Windows APIs only. Result is a stable machine fingerprint
// suitable for server-side license binding. Not a secret by itself.
// =============================================================================

#include <string>

namespace Security
{

class HwId
{
public:
    // Preferred: hardware profile GUID, fallback volume serial
    [[nodiscard]] static std::string Collect();

    // Hash form for compact storage / comparison
    [[nodiscard]] static std::string CollectHashed();
};

} // namespace Security
