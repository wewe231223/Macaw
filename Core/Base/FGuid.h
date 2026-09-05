#pragma once

#include "PCH.h"

struct FGuid
{
    uint32 A = 0;
    uint32 B = 0;
    uint32 C = 0;
    uint32 D = 0;

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