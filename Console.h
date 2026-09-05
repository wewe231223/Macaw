#pragma once

#include "pch.h"

struct FConsoleOutputHandle
{
    uint32_t Index = UINT32_MAX;

    bool IsValid() const
    {
        return Index != UINT32_MAX;
    }
};

namespace Console
{
    inline constexpr FConsoleOutputHandle STDOutHandle{ 0 };
    inline constexpr FConsoleOutputHandle STDErrorHandle{ 1 };

    void Print(FConsoleOutputHandle Handle, FString Message);
    size_t GetMessageCount(FConsoleOutputHandle Handle);
    const FString& GetMessageAt(FConsoleOutputHandle Handle, size_t Index);
}



