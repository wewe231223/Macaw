#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"

struct FViewportInput {
public:
    ReflectTypes(FViewportInput);

public:
    ReflectStruct(FViewportInput);

private:
    ReflectMembers();

public:
    FVector2 mMouse{};
    ReflectProperty(mMouse, "Mouse", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector2 mDelta{};
    ReflectProperty(mDelta, "Delta", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    float mWheel{};
    ReflectProperty(mWheel, "Wheel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mHovered{};
    ReflectProperty(mHovered, "Hovered", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mFocused{};
    ReflectProperty(mFocused, "Focused", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mLeftPressed{};
    ReflectProperty(mLeftPressed, "LeftPressed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mLeftDown{};
    ReflectProperty(mLeftDown, "LeftDown", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mLeftReleased{};
    ReflectProperty(mLeftReleased, "LeftReleased", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mRightDown{};
    ReflectProperty(mRightDown, "RightDown", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mMiddleDown{};
    ReflectProperty(mMiddleDown, "MiddleDown", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mAlt{};
    ReflectProperty(mAlt, "Alt", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mCycleGizmo{};
    ReflectProperty(mCycleGizmo, "CycleGizmo", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mFocusSelection{};
    ReflectProperty(mFocusSelection, "FocusSelection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mEscape{};
    ReflectProperty(mEscape, "Escape", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mDelete{};
    ReflectProperty(mDelete, "Delete", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mForward{};
    ReflectProperty(mForward, "Forward", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mBackward{};
    ReflectProperty(mBackward, "Backward", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mLeft{};
    ReflectProperty(mLeft, "Left", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mRight{};
    ReflectProperty(mRight, "Right", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mUp{};
    ReflectProperty(mUp, "Up", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mDown{};
    ReflectProperty(mDown, "Down", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mFast{};
    ReflectProperty(mFast, "Fast", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
