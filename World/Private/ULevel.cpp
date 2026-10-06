#include "pch.h"
#include "World/ULevel.h"
#include "World/AActor.h"
#include "World/UWorld.h"

ULevel::ULevel(UWorld& World)
	: mWorld(World) {
    SetOuter(&World);
    SetName(UObjectSystem::MakeUniqueObjectName(&World, FName{"PersistentLevel"}));
}

ULevel::~ULevel() = default;

UWorld& ULevel::GetWorld() const {
    return mWorld;
}

const TArray<std::unique_ptr<AActor>>& ULevel::GetActors() const {
    return mActors;
}

bool ULevel::CanChangeOuter(const UObject* NewOuter) const {
    return NewOuter == &mWorld;
}
