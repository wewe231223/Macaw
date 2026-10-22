#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

struct FRenderStats {
public:
    ReflectTypes(FRenderStats);

public:
    ReflectStruct(FRenderStats);

private:
    ReflectMembers();

public:
    Uint32 mDrawCalls{};
    ReflectProperty(mDrawCalls, "DrawCalls", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint32 mVisiblePrimitives{};
    ReflectProperty(mVisiblePrimitives, "VisiblePrimitives", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint32 mTransformUpdates{};
    ReflectProperty(mTransformUpdates, "TransformUpdates", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    std::size_t mGpuBytes{};
    ReflectProperty(mGpuBytes, "GpuBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
