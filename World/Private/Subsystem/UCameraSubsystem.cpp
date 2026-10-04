#include "pch.h"
#include "World/Subsystem/UCameraSubsystem.h"

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

const FTypeInfo* UCameraSubsystem::StaticTypeInfo() noexcept {
    static const FTypeInfo Information{"UCameraSubsystem", UWorldSubsystem::StaticTypeInfo(), +[]() -> std::unique_ptr<UObject> {
        return std::make_unique<UCameraSubsystem>();
    }};
    return &Information;
}

const FTypeInfo* UCameraSubsystem::GetTypeInfo() const noexcept {
    return StaticTypeInfo();
}
