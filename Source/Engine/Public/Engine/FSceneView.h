#pragma once

#include "Core/Math/FVector2.h"
#include "Core/Math/FMatrix.h"

struct FSceneView {
    FVector mLocation{};
    FVector mForward{1.0f, 0.0f, 0.0f};

    FMatrix mView{};
    FMatrix mProjection{};
    FMatrix mViewProjection{};

    FVector2 mSize{};
    float mFieldOfView{60.0f};
};
