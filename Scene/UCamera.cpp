#include "PCH.h"
#include "FMath.h"
#include "UCamera.h"

UCamera::UCamera()
    : FOV(FMath::DegreesToRadians(60.0f)),
    AspectRatio(16.0f / 9.0f),
    NearPlane(0.1f),
    FarPlane(1000.0f)
{
}

float UCamera::GetFOV() const { return FOV; }
float UCamera::GetAspectRatio() const { return AspectRatio; }
float UCamera::GetNearPlane() const { return NearPlane; }
float UCamera::GetFarPlane() const { return FarPlane; }

void UCamera::SetFOV(float InFOV)
{
    FOV = InFOV;
}
void UCamera::SetAspectRatio(float InAspectRatio)
{
    AspectRatio = InAspectRatio;
}
void UCamera::SetNearPlane(float InNearPlane)
{
    NearPlane = InNearPlane;
}
void UCamera::SetFarPlane(float InFarPlane)
{
    FarPlane = InFarPlane;
}

FMatrix UCamera::GetProjectionMatrix() const
{
    return FMatrix::CreatePerspectiveFieldOfView(
        FOV,
        AspectRatio,
        NearPlane,
        FarPlane);
}

FMatrix UCamera::GetViewMatrix() const
{
    FMatrix CameraWorld =
        GetTransform().GetWorldMatrix();

    return CameraWorld.Invert();
}

FMatrix UCamera::GetViewProjectionMatrix() const
{
    return GetViewMatrix() * GetProjectionMatrix();
}