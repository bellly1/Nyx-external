#pragma once

// =============================================================================
// Layer: Commercial protector integration (VMProtect / Themida / Enigma)
// =============================================================================
// These macros mark functions/regions for virtualization AFTER the Release build.
// Without the commercial SDK they compile to no-ops — zero runtime cost.
//
// VS Build: Release | x64 → open robloxini.exe in VMProtect → virtualize markers.
// See docs/SECURITY.md § "Commercial protectors".
// =============================================================================

#if defined(HAS_VMPROTECT_SDK)
#  include <VMProtectSDK.h>
#  define SEC_VMP_BEGIN(name)  VMProtectBeginUltra(name)
#  define SEC_VMP_END()       VMProtectEnd()
#  define SEC_VMP_BEGIN_MUTATION(name) VMProtectBeginMutation(name)
#elif defined(HAS_THEMIDA_SDK)
#  include <ThemidaSDK.h>
#  define SEC_VMP_BEGIN(name)  VM_START
#  define SEC_VMP_END()        VM_END
#  define SEC_VMP_BEGIN_MUTATION(name) VM_START
#else
#  define SEC_VMP_BEGIN(name)          ((void)0)
#  define SEC_VMP_END()                ((void)0)
#  define SEC_VMP_BEGIN_MUTATION(name) ((void)0)
#endif

// Annotate functions that should never be inlined across the protection boundary
#if defined(_MSC_VER)
#  define SEC_NOINLINE __declspec(noinline)
#else
#  define SEC_NOINLINE __attribute__((noinline))
#endif
