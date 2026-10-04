#include "pch.h"
#include "World/Subsystem/UWorldSubsystem.h"

void UWorldSubsystem::Initialize(UWorld* World) {
    if (mBInitialized || World == nullptr) {
        return;
    }

    this->mWorld = World;
    mBInitialized = true;
    OnInitialize();
}

void UWorldSubsystem::Deinitialize() {
    if (!mBInitialized) {
        return;
    }

    OnDeinitialize();
    mBInitialized = false;
    mWorld = nullptr;
}

UWorld* UWorldSubsystem::GetWorld() const {
    return mWorld;
}

bool UWorldSubsystem::IsInitialized() const {
    return mBInitialized;
}

void UWorldSubsystem::OnInitialize() {
}

void UWorldSubsystem::OnDeinitialize() {
}

const FTypeInfo* UWorldSubsystem::StaticTypeInfo() noexcept {
    static const FTypeInfo Information{"UWorldSubsystem", UObject::StaticTypeInfo(), nullptr};
    return &Information;
}

const FTypeInfo* UWorldSubsystem::GetTypeInfo() const noexcept {
    return StaticTypeInfo();
}
