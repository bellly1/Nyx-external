# Ship with max protection (required for real crack resistance)

In-source checks **raise the bar**. They do **not** replace VMProtect/Themida.
Your friend patching in x64dbg only dies hard after you **pack** critical code.

## 1. Build

```
Visual Studio → Release | x64
Output: bin\Release\robloxini.exe
```

Confirm:

- Generate Debug Info: **No**
- Control Flow Guard: **Yes** (already)
- Whole program optimization: **Yes**

## 2. VMProtect (critical functions only)

Open `robloxini.exe` in **VMProtect**.

### Virtualize (Ultra / Mutation+Virtualization)

| Function / area | Why |
|-----------------|-----|
| `EasyAuth::Api::License` | Key redeem |
| `EasyAuth::Api::Register` | Key on wire |
| `EasyAuth::Api::Login` | Session |
| `EasyAuth::Api::ValidateSession` | Live auth |
| `EasyAuth::Api::applyFromResponse` | Token accept logic |
| `Protect::MintSeal` / `VerifySealFast` / `Licensed` / `LicensedHot` | Local license seal |
| `Protect::LockRuntime` | Post-auth lock |
| `Harden::RuntimeClean` | Gate |
| `Harden::VerifyImageIntegrity` | Anti-patch |
| `Harden::CrackerToolPresent` | Tool detect |
| `Harden::BindCapability` / `CapabilityOk` | Feature key |

### Options (recommended)

- Virtualization: **Ultra** on list above  
- Memory protection: **On**  
- Import protection: **On**  
- Pack output: **On**  
- Strip debug / relocs as VMP suggests  

### Do NOT virtualize

- Full ImGui / D3D present path (perf + instability)  
- Entire `Engine::PlayersThread` (too hot)

## 3. Code sign (optional but good)

```
signtool sign /fd SHA256 /a /tr http://timestamp.digicert.com /td SHA256 robloxini.exe
```

## 4. Ship only the packed binary

```
robloxini.exe          ← VMProtect output only
Roboto-Medium.ttf
update_urls.txt
```

Never ship the **unpacked** Release build to friends.

## 5. Rotate often

Every update:

1. Bump `AutoUpdate::kAppVersion`  
2. Rebuild  
3. Re-pack with VMP (new mutation)  
4. `ShipUpdate.ps1`  

## What source already does

| Item | Status |
|------|--------|
| Kill process if x64dbg/IDA/CE/Scylla/… running | Yes |
| Anti-debug PEB / remote / hardware BP | Yes |
| Code CRC after auth (anti-patch) | Yes |
| HWID + EasyAuth server key | Yes |
| HTTPS + host allowlist EasyAuth.cc | Yes |
| API hash resolve / hooks / inject heuristics | Yes |
| Anti-dump PE wipe / working set | Partial |
| Full code virtualization / section encrypt | **VMProtect only** |
| Control-flow flattening / mutation | **VMProtect only** |

## Honest limit

Without VMProtect (or Themida/Enigma), a skilled reverser with time can still win.
With VMP on the license path + tool kill + integrity + server keys, casual “patch in x64dbg” usually dies.
