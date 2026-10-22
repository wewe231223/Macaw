#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "Engine/EPrimitiveType.h"

struct FPrimitiveSceneProxy {
public:
    ReflectTypes(FPrimitiveSceneProxy);

public:
    ReflectStruct(FPrimitiveSceneProxy);

private:
    ReflectMembers();

public:
    FGuid mId{};
    ReflectProperty(mId, "Id", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    EPrimitiveType mPrimitive{EPrimitiveType::Cube};
    ReflectProperty(mPrimitive, "Primitive", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FMatrix mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FBox mBounds{};
    ReflectProperty(mBounds, "Bounds", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
