#include "FGuid.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <combaseapi.h>
#include <format>

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
    return std::format("{:08X}-{:04X}-{:04X}-{:04X}-{:04X}{:08X}",
        A,
        B >> 16,        
        B & 0xFFFF,     
        C >> 16,       
        C & 0xFFFF,     
        D);
}