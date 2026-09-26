#pragma once

#include "Core/Common.h"
#include "Math/FMath.h"

enum class ELineDepthMode : Uint8 {
    DepthTested,
    Overlay
};

class ILineDrawContext {
public:
    virtual ~ILineDrawContext() = default;

public:
    virtual void AddLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels = 1.0f, ELineDepthMode DepthMode = ELineDepthMode::DepthTested) = 0;
};
