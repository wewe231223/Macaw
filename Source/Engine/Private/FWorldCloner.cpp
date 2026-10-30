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

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 액터와 소유 컴포넌트를 포함한 월드 객체의 비소유 목록을 수집합니다.
     * @param[in] Actors 직렬화하거나 수집할 액터 목록입니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
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

    /**
     * @brief 액터와 소유 컴포넌트를 포함한 월드 객체의 비소유 목록을 수집합니다.
     * @param[in] World 조회·설정 또는 직렬화할 월드입니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
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

    /**
     * @brief 객체 GUID의 중복을 검사하고 각 원본에 새 GUID를 할당합니다.
     * @param[in] Objects 수집할 객체 목록 또는 GUID별 비소유 객체 대응표입니다.
     * @param[in,out] Remapping 원본 GUID와 복제 GUID 사이의 대응표입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool BuildRemapping(const TArray<UObject*>& Objects, TMap<FGuid, FGuid, FGuidHash>& Remapping, FString& Error) {
        for (const UObject* Object : Objects) {
            if (!Object->GetGuid().IsValid() || !Remapping.emplace(Object->GetGuid(), FGuid{}).second) {
                Error = "Duplicate or invalid source identity";
                return false;
            }
        }

        return true;
    }

    /**
     * @brief GUID 대응표로 원본 객체와 복제 객체를 검증하여 복제 컨텍스트에 등록합니다.
     * @param[in] Originals 복제할 원본 객체의 비소유 주소 목록입니다.
     * @param[in] Copies 원본 목록과 대응하는 복제 객체의 비소유 주소 목록입니다.
     * @param[in] Remapping 원본 GUID와 복제 GUID 사이의 대응표입니다.
     * @param[in,out] Duplication 원본과 복제 객체의 대응 관계를 기록할 컨텍스트입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
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
