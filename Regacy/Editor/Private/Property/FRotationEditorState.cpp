#include "pch.h"
#include "Editor/Property/FRotationEditorState.h"

#include <cmath>

namespace {
    float RotationDistanceSquared(const FQuat& First, const FQuat& Second) {
        const float Sign{First.X * Second.X + First.Y * Second.Y + First.Z * Second.Z + First.W * Second.W < 0.0f ? -1.0f : 1.0f};
        const float X{First.X - Sign * Second.X};
        const float Y{First.Y - Sign * Second.Y};
        const float Z{First.Z - Sign * Second.Z};
        const float W{First.W - Sign * Second.W};

        return X * X + Y * Y + Z * Z + W * W;
    }

    float NearestAngle(float Angle, float Reference) {
        return Angle + 360.0f * std::round((Reference - Angle) / 360.0f);
    }

    FRotator MakeEulerRotation(const FQuat& Rotation, const FRotator& Reference) {
        FRotator Euler{Rotation.ToRotator()};

        if (std::abs(Euler.Pitch) >= 90.0f) {
            const float Sign{std::copysign(1.0f, Euler.Pitch)};

            Euler.Roll = NearestAngle(Euler.Roll + Sign * Reference.Yaw, Reference.Roll);
            Euler.Yaw = Reference.Yaw;
        }

        return Euler;
    }

    FRotator NearestRotation(const FRotator& Rotation, const FRotator& Reference) {
        return FRotator{NearestAngle(Rotation.Pitch, Reference.Pitch), NearestAngle(Rotation.Yaw, Reference.Yaw), NearestAngle(Rotation.Roll, Reference.Roll)};
    }

    float EulerDistanceSquared(const FRotator& First, const FRotator& Second) {
        const float Pitch{First.Pitch - Second.Pitch};
        const float Yaw{First.Yaw - Second.Yaw};
        const float Roll{First.Roll - Second.Roll};

        return Pitch * Pitch + Yaw * Yaw + Roll * Roll;
    }
}

const FRotator& FRotationEditorState::Update(const FQuat& Rotation) {
    if (mInitialized && RotationDistanceSquared(Rotation, mQuaternion) <= 1e-12f) {
        return mRotation;
    }

    FRotator Euler{MakeEulerRotation(Rotation, mRotation)};

    if (mInitialized) {
        const FRotator Primary{NearestRotation(Euler, mRotation)};
        const FRotator Alternate{NearestRotation(FRotator{180.0f - Euler.Pitch, Euler.Yaw + 180.0f, Euler.Roll + 180.0f}, mRotation)};

        Euler = EulerDistanceSquared(Primary, mRotation) <= EulerDistanceSquared(Alternate, mRotation) ? Primary : Alternate;
    }

    mRotation = Euler;
    mQuaternion = Rotation;
    mInitialized = true;

    return mRotation;
}

void FRotationEditorState::SetRotation(const FRotator& Rotation) {
    mRotation = Rotation;
    mQuaternion = FQuat::FromRotator(Rotation);
    mInitialized = true;
}
