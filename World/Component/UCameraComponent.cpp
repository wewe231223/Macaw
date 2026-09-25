#include "pch.h"
#include "UCameraComponent.h"

#include "World/AActor.h"
#include "World/UWorld.h"
#include "World/Subsystem/UCameraSubsystem.h"

UCameraComponent::UCameraComponent() {
    SetRelativeLocation({5.0f, 5.0f, 5.0f});
}

FMatrix UCameraComponent::GetViewMatrix() const {
    return (CameraBasis * GetComponentToWorld()).Invert();
}

FMatrix UCameraComponent::GetProjectionMatrix() const {
    return FMatrix::CreatePerspectiveFieldOfView(mFov, mAspectRatio, mNearPlane, mFarPlane);
}

FMatrix UCameraComponent::GetViewProjectionMatrix() const {
    return GetViewMatrix() * GetProjectionMatrix();
}

float UCameraComponent::GetFOV() const {
    return mFov;
}

float UCameraComponent::GetAspectRatio() const {
    return mAspectRatio;
}

float UCameraComponent::GetNearPlane() const {
    return mNearPlane;
}

float UCameraComponent::GetFarPlane() const {
    return mFarPlane;
}

void UCameraComponent::SetFOV(float InFOV) {
    mFov = InFOV;
}

void UCameraComponent::SetAspectRatio(float InAspectRatio) {
    mAspectRatio = InAspectRatio;
}

void UCameraComponent::SetNearPlane(float InNearPlane) {
    mNearPlane = InNearPlane;
}

void UCameraComponent::SetFarPlane(float InFarPlane) {
    mFarPlane = InFarPlane;
}

void UCameraComponent::OnRegister() {
    UActorComponent::OnRegister();
    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetCameraSubsystem().SetMainCamera(this);
    }
}

void UCameraComponent::OnUnregister() {
    UActorComponent::OnUnregister();

    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetCameraSubsystem().ClearMainCamera(this);
    }
}

void UCameraComponent::SetMoveSensitivity(float InMoveSensitivity) {
    mMoveSensitivity = InMoveSensitivity;
}

void UCameraComponent::SetRotationSensitivity(float InRotationSensitivity) {
    mRotationSensitivity = InRotationSensitivity;
}

float UCameraComponent::GetRotationSensitivity() const {
    return mRotationSensitivity;
}

float UCameraComponent::GetMoveSensitivity() const {
    return mMoveSensitivity;
}

void UCameraComponent::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);
    Archive.Serialize("FOV", mFov);
    Archive.Serialize("AspectRatio", mAspectRatio);
    Archive.Serialize("NearPlane", mNearPlane);
    Archive.Serialize("FarPlane", mFarPlane);
}
