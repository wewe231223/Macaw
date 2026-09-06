#include "PCH.h"
#include "UCameraComponent.h"

FMatrix UCameraComponent::GetViewMatrix() const
{
    return GetTransform().GetWorldMatrix().Invert();
}

FMatrix UCameraComponent::GetProjectionMatrix() const
{
    return FMatrix::CreatePerspectiveFieldOfView(
        FOV,
        AspectRatio,
        NearPlane,
        FarPlane
    );
}

FMatrix UCameraComponent::GetViewProjectionMatrix() const
{
    return GetViewMatrix() * GetProjectionMatrix();
}