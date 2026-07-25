# Nyx External

Visual Studio 2022 (v143) · C++20 · Release | x64

## Layout

```
RobloxInt.sln
RobloxInt/          # main app (src, include, offsets)
vendor/             # imgui + easyauth
bin/Release/        # build output (robloxini.exe)
lootlabs-keygen/    # optional key helper (Node) — not part of the C++ build
docs/               # security notes
```

## Build

Open `RobloxInt.sln` or:

```powershell
msbuild RobloxInt.sln /p:Configuration=Release /p:Platform=x64
```

Output: `bin\Release\robloxini.exe`

Ship with `Roboto-Medium.ttf` and `update_urls.txt` next to the exe.

## Update channel

```powershell
powershell -ExecutionPolicy Bypass -File .\ShipUpdate.ps1
```

## World tab

Ambience, Fog, and Brightness only (sticky Lighting writes).
