#include "PCH.h"
#include "FMath.h"
#include "FTransform.h"

FMatrix FTransform::GetWorldMatrix() const
{
    FMatrix S = FMatrix::CreateScale(Scale);

    FMatrix R = FMatrix::CreateFromYawPitchRoll(
        Rotation.y,  // Yaw
        Rotation.x,  // Pitch
        Rotation.z);

    FMatrix T = FMatrix::CreateTranslation(Position);

    return S * R * T;
}