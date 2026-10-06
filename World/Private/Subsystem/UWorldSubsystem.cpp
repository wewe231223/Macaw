#include "pch.h"
#include "World/Subsystem/UWorldSubsystem.h"
#include "World/UWorld.h"

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

bool UWorldSubsystem::CanChangeOuter(const UObject* NewOuter) const {
    return mWorld != nullptr ? NewOuter == mWorld : (NewOuter == nullptr || NewOuter->GetTypeInfo()->IsA(UWorld::StaticTypeInfo()));
}
