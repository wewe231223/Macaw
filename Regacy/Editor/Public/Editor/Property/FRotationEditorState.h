#pragma once

#include "Math/FMath.h"

class FRotationEditorState {
public:
    FRotationEditorState() = default;

public:
    const FRotator& Update(const FQuat& Rotation);
    void SetRotation(const FRotator& Rotation);

private:
    FRotator mRotation{};
    FQuat mQuaternion{};
    bool mInitialized{};
};
