#include "security/HwId.hpp"

#include <Windows.h>
#include <cstdio>
#include <string>

namespace Security
{

std::string HwId::Collect()
{
    HW_PROFILE_INFOA hw{};
    if (GetCurrentHwProfileA(&hw) && hw.szHwProfileGuid[0])
        return std::string(hw.szHwProfileGuid);

    DWORD serial = 0;
    GetVolumeInformationA("C:\\", nullptr, 0, &serial, nullptr, nullptr, nullptr, 0);
    char buf[32]{};
    sprintf_s(buf, "%08X", serial);
    return buf;
}

std::string HwId::CollectHashed()
{
    const std::string raw = Collect();
    uint64_t h = 14695981039346656037ull;
    for (unsigned char c : raw)
    {
        h ^= c;
        h *= 1099511628211ull;
    }
    h ^= GetCurrentProcessId() * 0x9E3779B97F4A7C15ull;
    char out[24]{};
    sprintf_s(out, "%016llX", static_cast<unsigned long long>(h));
    return out;
}

} // namespace Security
