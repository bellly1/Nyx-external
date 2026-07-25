# Ship a new robloxini build for auto-update via GitHub.
# 1) Bump kAppVersion in RobloxInt\include\AutoUpdate.hpp
# 2) Build Release x64
# 3) powershell -ExecutionPolicy Bypass -File .\ShipUpdate.ps1
# 4) If it patches AutoUpdate.hpp, rebuild once more
# 5) Give friends: robloxini.exe + Roboto-Medium.ttf + update_urls.txt

$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Exe  = Join-Path $Root "bin\Release\robloxini.exe"
$Hdr  = Join-Path $Root "RobloxInt\include\AutoUpdate.hpp"
$Repo = "https://github.com/bellly1/robloxini-updates"
$RawBase = "https://raw.githubusercontent.com/bellly1/robloxini-updates/main"

if (-not (Test-Path $Exe)) {
    Write-Host "[!] missing $Exe - build Release first" -ForegroundColor Red
    exit 1
}

# Read current kAppVersion from header
$ver = "0.0.0"
if (Test-Path $Hdr) {
    $txt = Get-Content $Hdr -Raw
    if ($txt -match 'kAppVersion\s*=\s*"([^"]+)"') {
        $ver = $Matches[1]
    }
}

Write-Host "[*] shipping robloxini v$ver" -ForegroundColor Cyan
$sizeMb = [math]::Round((Get-Item $Exe).Length / 1MB, 2)
Write-Host "[*] exe: $Exe ($sizeMb MB)"

# Copy exe as .bin for GitHub upload
$binCopy = Join-Path $env:TEMP "robloxini.bin"
Copy-Item $Exe $binCopy -Force
Write-Host "[*] prepared $binCopy for upload"

# Write version.txt locally
$verBody = "version=$ver`nexe=$RawBase/robloxini.bin`nenabled=1`n"
$verFile = Join-Path $env:TEMP "version.txt"
[System.IO.File]::WriteAllBytes($verFile, [Text.Encoding]::ASCII.GetBytes($verBody))
Write-Host "[*] version.txt content:`n$verBody"

# Push binaries to GitHub using git
$UpdatesDir = Join-Path $env:TEMP "robloxini_updates_push"
if (Test-Path $UpdatesDir) { Remove-Item $UpdatesDir -Recurse -Force }

Write-Host "[*] cloning update repo..."
git clone $Repo $UpdatesDir 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[!] git clone failed" -ForegroundColor Red
    exit 1
}

# Copy files
Copy-Item $binCopy (Join-Path $UpdatesDir "robloxini.bin") -Force
Copy-Item $verFile (Join-Path $UpdatesDir "version.txt") -Force

Push-Location $UpdatesDir
try {
    # Always attribute ships to bellly1 only (avoids extra GitHub contributors)
    git config user.name "bellly1"
    git config user.email "154699106+bellly1@users.noreply.github.com"
    git add robloxini.bin version.txt
    git -c user.name="bellly1" -c user.email="154699106+bellly1@users.noreply.github.com" commit -m "v$ver" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[!] git commit failed" -ForegroundColor Red
        exit 1
    }
    git push 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[!] git push failed" -ForegroundColor Red
        exit 1
    }
} finally {
    Pop-Location
    Remove-Item $UpdatesDir -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "[+] pushed robloxini.bin + version.txt to GitHub as v$ver" -ForegroundColor Green

# Patch baked-in defaults in AutoUpdate.hpp so next build uses GitHub URLs
if (Test-Path $Hdr) {
    $src = Get-Content $Hdr -Raw

    # Patch kAppVersion
    $src2 = [regex]::Replace(
        $src,
        'inline constexpr const char\* kAppVersion\s*=\s*"[^"]*";',
        "inline constexpr const char* kAppVersion = `"$ver`";")

    # Patch kDefaultVersionUrl to GitHub API
    $src2 = [regex]::Replace(
        $src2,
        'inline constexpr const char\* kDefaultVersionUrl\s*=\s*"[^"]*";',
        "inline constexpr const char* kDefaultVersionUrl =`r`n    `"$RawBase/version.txt`";")

    # Patch kDefaultExeUrl to raw GitHub
    $src2 = [regex]::Replace(
        $src2,
        'inline constexpr const char\* kDefaultExeUrl\s*=\s*"[^"]*";',
        "inline constexpr const char* kDefaultExeUrl =`r`n    `"$RawBase/robloxini.bin`";")

    # Patch kVersionFallbacks to use GitHub API + raw + jsDelivr
    $src2 = [regex]::Replace(
        $src2,
        'inline constexpr const char\* kVersionFallbacks\[\] = \{[\s\S]*?\};',
        ("inline constexpr const char* kVersionFallbacks[] = {`r`n" +
         "    `"$Repo/contents/version.txt`",`r`n" +
         "    `"$RawBase/version.txt`",`r`n" +
         "    `"https://cdn.jsdelivr.net/gh/bellly1/robloxini-updates@main/version.txt`",`r`n" +
         "};"),
        1)

    if ($src2 -ne $src) {
        [System.IO.File]::WriteAllText($Hdr, $src2)
        Write-Host "[+] patched AutoUpdate.hpp - REBUILD once more if you want the new exe online" -ForegroundColor Green
    }
}

# Write update_urls.txt next to release exe
$urlsPath = Join-Path (Split-Path $Exe) "update_urls.txt"
$urls = "# robloxini update channel`r`nversion=$Repo/contents/version.txt`r`nexe=$RawBase/robloxini.bin`r`n"
[System.IO.File]::WriteAllText($urlsPath, $urls)
Write-Host "[+] wrote $urlsPath"

Write-Host "[*] verifying version.txt..."
curl.exe -sL "$RawBase/version.txt"
Write-Host ""
Write-Host "[+] DONE." -ForegroundColor Green
Write-Host "    Give friends: robloxini.exe + Roboto-Medium.ttf + update_urls.txt"
Write-Host "    They must use THIS new build (v$ver)."
