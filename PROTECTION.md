# Protection map — Nyx / robloxini

Honest mapping of commercial-style protections vs what this project actually has.

## Legend

| Status | Meaning |
|--------|---------|
| **In-code** | Implemented in `Protect.hpp` / `Harden.hpp` / EasyAuth / MSVC flags |
| **Partial** | Lightweight or best-effort only |
| **External** | Requires VMProtect / Themida / signtool / CI — not reimplemented |

---

## Code protection

| Feature | Status | Notes |
|---------|--------|-------|
| Virtualization (full x86 VM) | **External** | Use VMProtect “Ultra” / Mutation+Virtualization on selected funcs after build |
| Mini stack-VM (selected checks) | **Partial** | `Harden::VmEvalSealFragment` — not a packer VM |
| Mutation | **External** | Packer |
| Control Flow Flattening | **External** | Packer / obfuscator |
| Function inlining/outlining | **Partial** | Compiler `/O2` + `__forceinline`; packer for aggressive |
| Code encryption / runtime decrypt | **External** | Packer section encryption |
| Instruction substitution | **External** | Packer |
| Opaque predicates | **In-code** | `Protect` + `Harden::OpaqueTrue/False` |
| Junk code insertion | **In-code** | `Junk` / `JunkCode` |
| String encryption | **In-code** | `skCrypt` / `XS` |
| Constant encryption | **In-code** | `ENC_U32` / `ENC_U64` |
| Import table protection | **Partial** | API hashing resolve; full IAT destroy = packer |
| API hashing / hiding | **In-code** | `Harden::ResolveByHash` |

## Anti-tamper

| Feature | Status |
|---------|--------|
| File / section integrity (self CRC) | **In-code** `CaptureImageIntegrity` / `VerifyImageIntegrity` |
| Memory integrity (code CRC) | **Partial** first 256KB of executable section |
| Self-checksumming | **In-code** periodic in `Harden::Tick` |
| Digital signature verification | **External** `signtool` + optional WinVerifyTrust later |
| Patch / hook detection | **In-code** inline JMP/INT3 checks on critical APIs |
| Encrypted code sections | **External** packer |

## Anti-debug

| Feature | Status |
|---------|--------|
| IsDebuggerPresent / remote | **In-code** |
| Hardware BP (DR0–3) | **In-code** |
| Software BP (INT3 on stubs) | **Partial** |
| Timing checks | **Partial** soft score only |
| Thread hiding | **In-code** `NtSetInformationThread` |
| Kernel debugger | **In-code** `NtQuerySystemInformation` |
| Parent process | **Partial** |
| Exception-based | **Partial** (removed noisy CloseHandle trap) |

## Anti-analysis / anti-VM

| Feature | Status |
|---------|--------|
| Anti-disassembly / decompilation | **External** packer + CFF |
| Symbol / PDB strip | **In-code** Release: no debug info in vcxproj |
| Fake control flow / fake funcs | **Partial** opaque paths |
| Dynamic API resolution | **In-code** |
| VMware / VBox / Hyper-V artifacts | **Partial** registry + CPUID (soft; won’t sole-kill) |
| Sandbox user heuristics | **Partial** |

## Memory / process

| Feature | Status |
|---------|--------|
| DEP / NX | **In-code** + linker |
| ASLR | **In-code** `/DYNAMICBASE` |
| CFG | **In-code** vcxproj `ControlFlowGuard` |
| Stack cookies `/GS` | **In-code** MSVC default |
| Guard pages / mem encrypt | **Partial** `SecureAlloc` helper only |
| Single-instance | **In-code** mutex |
| DLL injection heuristics | **Partial** module name blacklist |
| Process hollowing / APC / manual map | **External** / advanced — not fully covered |

## License

| Feature | Status |
|---------|--------|
| Online activation | **In-code** EasyAuth |
| HWID binding | **In-code** |
| Server validate / heartbeat | **In-code** |
| Revocation / subscription | **Server-side** EasyAuth panel |
| Offline activation | **External** panel feature if offered |

## Recommended post-build pipeline

```text
1. Build Release | x64  (robloxini.exe)
2. Optional: edit VMProtect project — virtualize:
     - Protect::VerifySeal*
     - EasyAuth::Api::License / Login / Register
     - Harden::RescanThreats
3. VMProtect / Themida pack
4. signtool sign /fd SHA256 ...
5. Ship: robloxini.exe + Roboto-Medium.ttf + update_urls.txt
```

## Important

- Enabling **every** anti-VM / anti-tool check as a hard kill will false-positive on real users (Hyper-V, Process Explorer, overlays).
- Current policy: **hard trip** only on debugger attach + image integrity failure + severe hook/inject score.
- Packing with VMProtect is what actually stops casual cracking; in-source checks raise the bar and protect the license path.
