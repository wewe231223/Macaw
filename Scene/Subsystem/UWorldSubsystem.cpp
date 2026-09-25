#include "pch.h"

#include "UWorldSubsystem.h"

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
