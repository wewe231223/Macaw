#include "pch.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/FWorldContext.h"
#include "World/UWorld.h"

FWorldContext::FWorldContext(EWorldType WorldType)
	: mWorld(NewObject<UWorld>()) {
    mWorld->Initialize(WorldType);
}

FWorldContext::~FWorldContext() {
    mWorld->CleanupWorld();
    UObjectSystem::Unregister(mWorld.get(), mWorld->GetHandle());
}

UWorld& FWorldContext::GetWorld() {
    return *mWorld;
}

const UWorld& FWorldContext::GetWorld() const {
    return *mWorld;
}

EWorldType FWorldContext::GetWorldType() const {
    return mWorld->GetWorldType();
}
