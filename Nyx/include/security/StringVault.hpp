#pragma once

// =============================================================================
// Layer: String encryption (compile-time skCrypt + RAII clear)
// =============================================================================
// Sensitive literals should not sit as plain ASCII in the binary.
// Decrypt only at the use site; wipe temporary std::string when done.
// =============================================================================

#include "skStr.h"
#include "SecureBuffer.hpp"
#include <string>

namespace Security
{

// Decrypt a compile-time encrypted string into a SecureBuffer (wiped on destroy)
#define SEC_STR(lit) (::Security::SecureBuffer(std::string(skCrypt(lit).decrypt())))

// Temporary plain string — prefer SEC_STR for secrets; use for short UI/log paths
#define SEC_CSTR(lit) (skCrypt(lit).decrypt())

} // namespace Security
