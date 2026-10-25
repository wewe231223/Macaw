/**
 * @file FWorldCloner.cpp
 * @brief 새 식별자 발급과 참조 재연결을 포함한 월드 복제를 구현합니다.
 * @ingroup Engine
 */

#include "Engine/FWorldCloner.h"
#include "Engine/FWorldSerializer.h"
#include "Engine/FWorldContext.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Serialization/MemoryArchive.h"

#include <utility>

namespace {
    TArray<UObject*> CollectObjects(const TArray<AActor*>& Actors) {
        TArray<UObject*> Objects{};

        for (AActor* Actor : Actors) {
            Objects.push_back(Actor);

            for (const auto& Component : Actor->GetComponents()) {
                Objects.push_back(Component.get());
            }
        }

        return Objects;
    }

    TArray<UObject*> CollectObjects(UWorld* World) {
        TArray<AActor*> Actors{};

        for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
            if (!Actor->IsPendingDestroy()) {
                Actors.push_back(Actor.get());
            }
        }

        auto Objects{CollectObjects(Actors)};
        Objects.push_back(World);
        Objects.push_back(World->GetPersistentLevel());
        return Objects;
    }

    bool BuildRemapping(const TArray<UObject*>& Objects, TMap<FGuid, FGuid, FGuidHash>& Remapping, FString& Error) {
        for (const UObject* Object : Objects) {
            if (!Object->GetGuid().IsValid() || !Remapping.emplace(Object->GetGuid(), FGuid{}).second) {
                Error = "Duplicate or invalid source identity";
                return false;
            }
        }

        return true;
    }

    bool BuildDuplication(const TArray<UObject*>& Originals, const TArray<UObject*>& Copies, const TMap<FGuid, FGuid, FGuidHash>& Remapping, FObjectDuplicationContext& Duplication, FString& Error) {
        TMap<FGuid, UObject*, FGuidHash> Objects{};

        for (UObject* Copy : Copies) {
            if (!Objects.emplace(Copy->GetGuid(), Copy).second) {
                Error = "Duplicate identity in cloned objects";
                return false;
            }
        }

        if (Originals.size() != Copies.size()) {
            Error = "Duplicated object count does not match the source";
            return false;
        }

        for (const UObject* Original : Originals) {
            const auto Guid{Remapping.find(Original->GetGuid())};
            const auto Copy{Guid != Remapping.end() ? Objects.find(Guid->second) : Objects.end()};

            if (Copy == Objects.end() || !Duplication.Add(Original, Copy->second)) {
                Error = "Cannot reconnect duplicated object";
                return false;
            }
        }

        return true;
    }
}

/**
 * @brief 월드, 영속 레벨, 액터와 컴포넌트를 독립 컨텍스트로 복제합니다.
 */
std::unique_ptr<FWorldContext> FWorldCloner::Clone(const FWorldContext* Source, EWorldType Type, FObjectDuplicationContext* Duplication, FString& Error) {
    Error.clear();

    if (Duplication != nullptr) {
        Duplication->Clear();
    }

    UWorld* SourceWorld{Source != nullptr ? Source->GetWorld() : nullptr};

    if (Duplication == nullptr || Type > EWorldType::EditorPreview || SourceWorld == nullptr || SourceWorld->IsDispatching() || SourceWorld->GetPersistentLevel() == nullptr) {
        Error = "Source world or target world type is invalid";
        return nullptr;
    }

    UWorld::FDispatchScope SourceScope{SourceWorld};
    auto Context{std::make_unique<FWorldContext>(Type)};
    UWorld* World{Context->GetWorld()};
    UWorld::FDispatchScope DuplicateScope{World};
    const auto Originals{CollectObjects(SourceWorld)};
    TMap<FGuid, FGuid, FGuidHash> Remapping{};

    if (!BuildRemapping(Originals, Remapping, Error)) {
        return nullptr;
    }

    Remapping.at(SourceWorld->GetGuid()) = World->GetGuid();
    Remapping.at(SourceWorld->GetPersistentLevel()->GetGuid()) = World->GetPersistentLevel()->GetGuid();
    TArray<Uint8> Bytes{};
    FMemoryArchive Writer{Bytes};
    Writer.SetObjectGuidRemapping(&Remapping);
    FWorldSerializer::Serialize(Writer, Source);

    if (Writer.HasError() || CollectObjects(SourceWorld) != Originals || !SourceWorld->mPendingActors.empty() || SourceWorld->mShutdownRequested || SourceWorld->mEndPlayRequested) {
        Error = Writer.HasError() ? Writer.GetError() : "Source world changed during serialization";
        return nullptr;
    }

    FMemoryArchive Reader{static_cast<const TArray<Uint8>&>(Bytes)};
    Reader.SetObjectGuidRemapping(&Remapping);
    FWorldSerializer::Serialize(Reader, Context.get());
    FObjectDuplicationContext Objects{};

    if (Reader.HasError() || Reader.GetRemainingBytes() != 0 || !BuildDuplication(Originals, CollectObjects(World), Remapping, Objects, Error)) {
        if (Error.empty()) {
            Error = Reader.HasError() ? Reader.GetError() : "Unread world duplication data";
        }

        return nullptr;
    }

    for (const auto& [Original, Duplicate] : Objects.GetObjects()) {
        if (!Duplicate->PostDuplicate(Original, &Objects)) {
            Error = "Duplicate fixup failed: " + Original->GetClass()->GetName();
            return nullptr;
        }

        if (!World->mPendingActors.empty() || World->mShutdownRequested || World->mEndPlayRequested || !SourceWorld->mPendingActors.empty() || SourceWorld->mShutdownRequested || SourceWorld->mEndPlayRequested) {
            Error = "Duplicate fixup requested world teardown";
            return nullptr;
        }
    }

    *Duplication = std::move(Objects);
    return Context;
}

TArray<FObjectHandle> FWorldCloner::DuplicateActors(UWorld* World, const TArray<FObjectHandle>& Actors, FString& Error) {
    Error.clear();

    if (World == nullptr || !World->CanAddActor() || World->IsDispatching() || Actors.empty()) {
        Error = "Invalid actor duplication request";
        return {};
    }

    UWorld::FDispatchScope Scope{World};
    TArray<AActor*> Sources{};

    for (FObjectHandle Handle : Actors) {
        auto* Source{Cast<AActor>(FObjectRegistry::Get()->Resolve(Handle))};

        if (Source == nullptr || Source->GetWorld() != World || Source->IsPendingDestroy()) {
            Error = "Invalid source actor";
            return {};
        }

        Sources.push_back(Source);
    }

    const auto Originals{CollectObjects(Sources)};
    TMap<FGuid, FGuid, FGuidHash> Remapping{};

    if (!BuildRemapping(Originals, Remapping, Error)) {
        return {};
    }

    Remapping.emplace(World->GetGuid(), World->GetGuid());
    Remapping.emplace(World->GetPersistentLevel()->GetGuid(), World->GetPersistentLevel()->GetGuid());
    TArray<Uint8> Bytes{};
    FMemoryArchive Writer{Bytes};
    Writer.SetObjectGuidRemapping(&Remapping);
    FWorldSerializer::SaveActors(Writer, Sources);

    if (Writer.HasError() || CollectObjects(Sources) != Originals || !World->mPendingActors.empty() || !World->CanAddActor() || World->mEndPlayRequested) {
        Error = Writer.HasError() ? Writer.GetError() : "Source actors changed during serialization";
        return {};
    }

    FMemoryArchive Reader{static_cast<const TArray<Uint8>&>(Bytes)};
    Reader.SetObjectGuidRemapping(&Remapping);
    auto Copies{FWorldSerializer::LoadActors(Reader)};
    TArray<AActor*> Loaded{};

    for (const auto& Copy : Copies) {
        Loaded.push_back(Copy.get());
    }

    FObjectDuplicationContext Objects{};

    if (Reader.HasError() || Reader.GetRemainingBytes() != 0 || !BuildDuplication(Originals, CollectObjects(Loaded), Remapping, Objects, Error)) {
        if (Error.empty()) {
            Error = Reader.HasError() ? Reader.GetError() : "Unread actor duplication data";
        }

        return {};
    }

    for (const auto& [Original, Duplicate] : Objects.GetObjects()) {
        if (!Duplicate->PostDuplicate(Original, &Objects)) {
            Error = "Actor reference fixup failed";
            return {};
        }

        if (!World->mPendingActors.empty() || !World->CanAddActor() || World->mEndPlayRequested) {
            Error = "Actor duplication requested world teardown";
            return {};
        }
    }

    TArray<FObjectHandle> Result{};

    for (auto& Copy : Copies) {
        const FObjectHandle Handle{Copy->GetHandle()};

        if (!World->GetPersistentLevel()->AddActor(std::move(Copy))) {
            for (FObjectHandle Added : Result) {
                World->DestroyActor(Added);
            }

            Error = "Duplicated actor registration failed";
            return {};
        }

        Result.push_back(Handle);
    }

    return Result;
}
