#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Engine/Serialization/FWorldCloner.h"
#include "Engine/Serialization/FWorldSerializer.h"
#include "Serialization/FArchiveMemory.h"
#include "World/UWorld.h"

std::unique_ptr<UWorld> FWorldCloner::Clone(UWorld& SourceWorld, EWorldType WorldType) {
    if (!FWorldSerializer::CanSerialize(SourceWorld) || FAssetRegistry::TryGet() == nullptr) {
        return {};
    }

    TArray<Uint8> Bytes{};
    FArchiveMemory SaveArchive{Bytes};

    if (!FWorldSerializer::Save(SourceWorld, SaveArchive)) {
        return {};
    }

    std::unique_ptr<UWorld> World{NewObject<UWorld>()};

    World->Initialize(WorldType);

    const TArray<Uint8>& SavedBytes{Bytes};
    FArchiveMemory LoadArchive{SavedBytes};
    const FWorldSerializer::FLoadScope LoadScope{*World};

    if (!FWorldSerializer::LoadInternal(*World, LoadArchive, &SourceWorld) || LoadArchive.HasError() || LoadArchive.GetRemainingBytes() != 0) {
        return {};
    }

    return World;
}
