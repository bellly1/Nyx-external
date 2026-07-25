#pragma once

#include "AutoUpdate.hpp"
#include "skStr.h"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>

namespace Fonts
{
inline std::filesystem::path ExeDir()
{
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
}

inline std::filesystem::path RobotoPath()
{
    return ExeDir() / L"Roboto-Medium.ttf";
}

inline bool LooksLikeTtf(const std::filesystem::path& path)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return false;
    const auto sz = std::filesystem::file_size(path, ec);
    if (ec || sz < 20000)
        return false;

    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    char magic[4]{};
    in.read(magic, 4);

    const bool ttf = (magic[0] == 0x00 && magic[1] == 0x01 && magic[2] == 0x00 && magic[3] == 0x00);
    const bool otf = (magic[0] == 'O' && magic[1] == 'T' && magic[2] == 'T' && magic[3] == 'O');
    const bool trueTag = (magic[0] == 't' && magic[1] == 'r' && magic[2] == 'u' && magic[3] == 'e');
    return ttf || otf || trueTag;
}

inline bool EnsureRobotoDownloaded()
{
    const auto dest = RobotoPath();
    if (LooksLikeTtf(dest))
        return true;

    std::cout << skCrypt("[*] downloading fonts...\n").decrypt();

    const char* urls[] = {
        "https://raw.githubusercontent.com/bellly1/robloxini-updates/main/Roboto-Medium.ttf",
        "https://cdn.jsdelivr.net/gh/bellly1/robloxini-updates@main/Roboto-Medium.ttf",
        "https://github.com/googlefonts/roboto/raw/main/src/hinted/Roboto-Medium.ttf",
        "https://cdn.jsdelivr.net/gh/googlefonts/roboto@main/src/hinted/Roboto-Medium.ttf",
    };

    for (const char* url : urls)
    {
        if (!url || !*url) continue;
        if (AutoUpdate::DownloadFile(url, dest) && LooksLikeTtf(dest))
        {
            std::cout << skCrypt("[+] fonts ready\n").decrypt();
            return true;
        }
        std::error_code ec;
        std::filesystem::remove(dest, ec);
    }

    std::cout << skCrypt("[!] font download failed (default font will be used)\n").decrypt();
    return false;
}
}

