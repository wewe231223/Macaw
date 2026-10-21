#pragma once

#include "Core/Guid.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "Engine/EPrimitiveType.h"

struct FPrimitiveSceneProxy {
    FGuid mId{};
    EPrimitiveType mPrimitive{EPrimitiveType::Cube};

    FMatrix mWorld{};
    FBox mBounds{};
};
