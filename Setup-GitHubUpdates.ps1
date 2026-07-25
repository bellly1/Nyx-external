# ============================================================
#  ONE-TIME setup so friends auto-update works on blocked WiFi
#  (paste.rs / catbox are often blocked; GitHub almost never is)
#
#  Before running:
#    1. Make a free GitHub account: https://github.com/signup
#    2. Install Git: https://git-scm.com/download/win  (if needed)
#    3. Create a PUBLIC empty repo named: robloxini-updates
#       https://github.com/new  (Public, no README required)
#    4. Install GitHub CLI optional: winget install GitHub.cli
#       then:  gh auth login
#
#  Then run:
#    powershell -ExecutionPolicy Bypass -File .\Setup-GitHubUpdates.ps1
# ============================================================

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Cdn  = Join-Path $Root "update-cdn"
$Hdr  = Join-Path $Root "RobloxInt\include\AutoUpdate.hpp"
$Exe  = Join-Path $Root "bin\Release\robloxini.exe"

Write-Host ""
Write-Host "  robloxini - GitHub update setup" -ForegroundColor Cyan
Write-Host ""

$user = Read-Host "Your GitHub username (exact)"
if ([string]::IsNullOrWhiteSpace($user)) {
    Write-Host "[!] username required" -ForegroundColor Red
    exit 1
}
$repo = "robloxini-updates"
$remote = "https://github.com/$user/$repo.git"

# Build latest into update-cdn
New-Item -ItemType Directory -Force -Path $Cdn | Out-Null
if (-not (Test-Path $Exe)) {
    Write-Host "[!] build Release first (missing $Exe)" -ForegroundColor Red
    exit 1
}
Copy-Item $Exe (Join-Path $Cdn "robloxini.bin") -Force

# Read version
$ver = "1.0.3"
if (Test-Path $Hdr) {
    $txt = Get-Content $Hdr -Raw
    if ($txt -match 'kAppVersion\s*=\s*"([^"]+)"') { $ver = $Matches[1] }
}

$versionUrl = "https://raw.githubusercontent.com/$user/$repo/main/version.txt"
$exeUrl     = "https://raw.githubusercontent.com/$user/$repo/main/robloxini.bin"
$jsv        = "https://cdn.jsdelivr.net/gh/$user/${repo}@main/version.txt"
$jse        = "https://cdn.jsdelivr.net/gh/$user/${repo}@main/robloxini.bin"

$verBody = "version=$ver`nexe=$exeUrl`n"
[System.IO.File]::WriteAllBytes((Join-Path $Cdn "version.txt"), [Text.Encoding]::ASCII.GetBytes($verBody))

# Init git repo in update-cdn
Push-Location $Cdn
if (-not (Test-Path ".git")) {
    git init -b main
}
git add -A
git status
git -c user.email="robloxini@local" -c user.name="robloxini" commit -m "update v$ver" 2>$null
# remote
$existing = git remote get-url origin 2>$null
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($existing)) {
    git remote add origin $remote
} else {
    git remote set-url origin $remote
}

Write-Host ""
Write-Host "[*] Pushing to $remote ..." -ForegroundColor Yellow
Write-Host "    (browser may ask you to log in to GitHub)"
git push -u origin main
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "[!] Push failed. Do this manually:" -ForegroundColor Red
    Write-Host "    1. Create public repo: https://github.com/new  name=$repo  Public"
    Write-Host "    2. cd `"$Cdn`""
    Write-Host "    3. git push -u origin main"
    Write-Host ""
    Pop-Location
    exit 1
}
Pop-Location

# Patch AutoUpdate.hpp defaults
if (Test-Path $Hdr) {
    $src = Get-Content $Hdr -Raw
    $src2 = [regex]::Replace($src,
        'inline constexpr const char\* kDefaultVersionUrl\s*=\s*"[^"]*";',
        "inline constexpr const char* kDefaultVersionUrl =`r`n    `"$versionUrl`";")
    $src2 = [regex]::Replace($src2,
        'inline constexpr const char\* kDefaultExeUrl\s*=\s*"[^"]*";',
        "inline constexpr const char* kDefaultExeUrl =`r`n    `"$exeUrl`";")
    $src2 = [regex]::Replace($src2,
        'inline constexpr const char\* kVersionFallbacks\[\] = \{[\s\S]*?\};',
        ("inline constexpr const char* kVersionFallbacks[] = {`r`n" +
         "    `"$jsv`",`r`n" +
         "    `"$versionUrl`",`r`n" +
         "};"),
        1)
    [System.IO.File]::WriteAllText($Hdr, $src2)
    Write-Host "[+] patched AutoUpdate.hpp" -ForegroundColor Green
}

# update_urls.txt for friends
$urlsPath = Join-Path (Split-Path $Exe) "update_urls.txt"
$urls = @"
# GitHub update channel - keep next to robloxini.exe
version=$versionUrl
exe=$exeUrl
# mirror (jsDelivr CDN - often faster)
# version=$jsv
# exe=$jse
"@
[System.IO.File]::WriteAllText($urlsPath, $urls)

Write-Host ""
Write-Host "[+] GitHub update channel ready!" -ForegroundColor Green
Write-Host "    version: $versionUrl"
Write-Host "    exe:     $exeUrl"
Write-Host ""
Write-Host "NEXT STEPS:"
Write-Host "  1. Rebuild Release in Visual Studio / MSBuild"
Write-Host "  2. Send friend: robloxini.exe + Roboto-Medium.ttf + update_urls.txt"
Write-Host "  3. Each future update: bump version, build, run ShipUpdate.ps1"
Write-Host ""
