#pragma once

#include "Engine/FDrawLine.h"

class FPrimitiveDrawInterface {
public:
    void DrawLine(const FVector& Start, const FVector& End, const FVector& Color, bool DepthTest = false);
    void Clear();

    const TArray<FDrawLine>& GetLines() const;

private:
    TArray<FDrawLine> mLines{};
};
