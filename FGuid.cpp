#include "pch.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <combaseapi.h>
#include <format>
#include <string>

#include "FGuid.h"

FGuid FGuid::NewGuid()
{
    FGuid NewGuid = { 0, 0, 0, 0 };

    GUID WinGuid;
    if (CoCreateGuid(&WinGuid) == S_OK)
    {
        memcpy(&NewGuid, &WinGuid, sizeof(FGuid));
    }

    return NewGuid;
}

FString FGuid::ToString() const
{
    const std::string Formatted = std::format(
        "{:08X}-{:04X}-{:04X}-{:04X}-{:04X}{:08X}",
        A,
        B >> 16,
        B & 0xFFFF,
        C >> 16,
        C & 0xFFFF,
        D);

    return FString(Formatted.begin(), Formatted.end());
}
