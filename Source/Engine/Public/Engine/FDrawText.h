#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"

struct FDrawText {
public:
    ReflectTypes(FDrawText);

public:
    ReflectStruct(FDrawText);

private:
    ReflectMembers();

public:
    FVector mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mText{};
    ReflectProperty(mText, "Text", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mSize{0.5f};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector2 mScale{1.0f, 1.0f};
    ReflectProperty(mScale, "Scale", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mScreenSpace{};
    ReflectProperty(mScreenSpace, "ScreenSpace", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mDepthTest{true};
    ReflectProperty(mDepthTest, "DepthTest", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
