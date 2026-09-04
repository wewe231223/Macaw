#pragma once

#include "pch.h"

#include <cstdint>

struct FGuid
{
    uint32_t A = 0;
    uint32_t B = 0;
    uint32_t C = 0;
    uint32_t D = 0;

    static FGuid NewGuid();
    FString ToString() const;

    inline bool IsValid() const
    {
        return (A | B | C | D) != 0;
    }

    inline bool operator==(const FGuid& Other) const
    {
        return (A == Other.A) && (B == Other.B) && (C == Other.C) && (D == Other.D);
    }

    inline bool operator!=(const FGuid& Other) const
    {
        return !(*this == Other);
    }
};