#pragma once

#include "Common.h"

struct FPoint {
    Int32 mX{0};
    Int32 mY{0};
};

struct FRect {
    FPoint mMin{};
    FPoint mMax{};

    Int32 GetWidth() const;

    Int32 GetHeight() const;

    bool IsEmpty() const;

    bool Contains(const FPoint& Point) const;
};
