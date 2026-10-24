#pragma once

#include "Core/ReflectionMacros.h"

struct FGizmoSettings {
public:
    ReflectTypes(FGizmoSettings);

public:
    ReflectStruct(FGizmoSettings);

private:
    ReflectMembers();

public:
    bool mTranslationSnap{};
    ReflectProperty(mTranslationSnap, "TranslationSnap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mRotationSnap{};
    ReflectProperty(mRotationSnap, "RotationSnap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mScaleSnap{};
    ReflectProperty(mScaleSnap, "ScaleSnap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mSurfaceSnap{};
    ReflectProperty(mSurfaceSnap, "SurfaceSnap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mAlignToSurface{};
    ReflectProperty(mAlignToSurface, "AlignToSurface", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mTranslationStep{1.0f};
    ReflectProperty(mTranslationStep, "TranslationStep", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mRotationStep{15.0f};
    ReflectProperty(mRotationStep, "RotationStep", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mScaleStep{0.1f};
    ReflectProperty(mScaleStep, "ScaleStep", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mSurfaceOffset{};
    ReflectProperty(mSurfaceOffset, "SurfaceOffset", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mSize{1.0f};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
