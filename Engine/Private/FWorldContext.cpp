#include "pch.h"
#include "Engine/FWorldContext.h"
#include "World/UWorld.h"

FWorldContext::FWorldContext(EWorldType WorldType)
	: mWorld{std::make_unique<UWorld>()} {
    mWorld->Initialize(WorldType);
    UObjectSystem::Register(mWorld.get());
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
