#pragma once

#include "Core/Types.h"

struct FGuid {
public:
    static FGuid NewGuid();
    static bool Parse(const FString& Text, FGuid& Result);
    FString ToString() const;
    bool IsValid() const;
    bool operator==(const FGuid& Other) const;

public:
    std::array<Uint8, 16> mBytes{};
};
