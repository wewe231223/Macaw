#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FDrawLine.h"

class FPrimitiveDrawInterface {
public:
    ReflectTypes(FPrimitiveDrawInterface);

public:
    ReflectStruct(FPrimitiveDrawInterface);

    void DrawLine(const FVector& Start, const FVector& End, const FVector& Color, bool DepthTest = false);
    void Clear();

    const TArray<FDrawLine>& GetLines() const;

private:
    ReflectMembers();

private:
    TArray<FDrawLine> mLines{};
    ReflectProperty(mLines, "Lines", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
