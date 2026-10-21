#pragma once

#include "Core/Math/FVector2.h"

struct FViewportInput {
    FVector2 mMouse{};
    FVector2 mDelta{};
    float mWheel{};

    bool mHovered{};
    bool mFocused{};

    bool mLeftPressed{};
    bool mLeftDown{};
    bool mLeftReleased{};
    bool mRightDown{};
    bool mMiddleDown{};

    bool mAlt{};

    bool mCycleGizmo{};
    bool mFocusSelection{};
    bool mEscape{};
    bool mDelete{};

    bool mForward{};
    bool mBackward{};
    bool mLeft{};
    bool mRight{};
    bool mUp{};
    bool mDown{};
    bool mFast{};
};
