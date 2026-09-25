#include "pch.h"

#include "UCameraSubsystem.h"

void UCameraSubsystem::SetMainCamera(UCameraComponent* Camera) {
    mMainCamera = Camera;
}

void UCameraSubsystem::ClearMainCamera(UCameraComponent* Camera) {
    if (mMainCamera == Camera) {
        mMainCamera = nullptr;
    }
}

UCameraComponent* UCameraSubsystem::GetMainCamera() const {
    return mMainCamera;
}

void UCameraSubsystem::OnDeinitialize() {
    mMainCamera = nullptr;
}
