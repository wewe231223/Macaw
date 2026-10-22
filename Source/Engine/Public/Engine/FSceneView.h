#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FMatrix.h"

struct FSceneView {
public:
    ReflectTypes(FSceneView);

public:
    ReflectStruct(FSceneView);

private:
    ReflectMembers();

public:
    FVector mLocation{};
    ReflectProperty(mLocation, "Location", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector mForward{1.0f, 0.0f, 0.0f};
    ReflectProperty(mForward, "Forward", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FMatrix mView{};
    ReflectProperty(mView, "View", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FMatrix mProjection{};
    ReflectProperty(mProjection, "Projection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FMatrix mViewProjection{};
    ReflectProperty(mViewProjection, "ViewProjection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FVector2 mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    float mFieldOfView{60.0f};
    ReflectProperty(mFieldOfView, "FieldOfView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
