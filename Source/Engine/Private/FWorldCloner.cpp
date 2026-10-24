/**
 * @file FWorldCloner.cpp
 * @brief 새 식별자 발급과 참조 재연결을 포함한 월드 복제를 구현합니다.
 * @ingroup Engine
 */

#include "Engine/FWorldCloner.h"
#include "Engine/FWorldContext.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"

#include <utility>

/**
 * @brief 객체 식별자와 Transient를 제외한 리플렉션 프로퍼티를 복사합니다.
 */
bool FWorldCloner::CopyProperties(const UObject* Source, UObject* Destination, FString& Error) {
    const FProperty* Identity{UObject::StaticClass()->FindClassProperty("Id")};

    for (const FProperty* Property : Source->GetClass()->GetAllProperties()) {
        if (Property == Identity || Property->HasFlag(EPropertyFlags::Transient)) {
            continue;
        }

        if (!Property->CopyValue(Destination, Source)) {
            Error = "Cannot duplicate property: " + Source->GetClass()->GetName() + "." + Property->GetName();
            return false;
        }
    }

    return true;
}

/**
 * @brief 원본 클래스의 객체를 생성하고 등록한 뒤 프로퍼티를 복사합니다.
 */
std::unique_ptr<UObject> FWorldCloner::CreateObject(const UObject* Source, FObjectDuplicationContext* Duplication, FString& Error) {
    auto Object{Source->GetClass()->CreateInstance()};

    if (Object == nullptr || Object->GetClass() != Source->GetClass() || !Duplication->Add(Source, Object.get())) {
        Error = "Cannot instantiate duplicate: " + Source->GetClass()->GetName();
        return nullptr;
    }

    if (!CopyProperties(Source, Object.get(), Error)) {
        return nullptr;
    }

    return Object;
}

/**
 * @brief 월드, 영속 레벨, 액터와 컴포넌트를 독립 컨텍스트로 복제합니다.
 */
std::unique_ptr<FWorldContext> FWorldCloner::Clone(const FWorldContext* Source, EWorldType Type, FObjectDuplicationContext* Duplication, FString& Error) {
    Error.clear();

    if (Duplication != nullptr) {
        Duplication->Clear();
    }

    const UWorld* SourceWorld{Source != nullptr ? Source->GetWorld() : nullptr};

    if (Duplication == nullptr || Type > EWorldType::EditorPreview || SourceWorld == nullptr || SourceWorld->IsDispatching() || SourceWorld->GetPersistentLevel() == nullptr) {
        Error = "Source world or target world type is invalid";
        return nullptr;
    }

    UWorld::FDispatchScope SourceScope{Source->GetWorld()};
    auto Context{std::make_unique<FWorldContext>(Type)};
    FObjectDuplicationContext Objects{};
    UWorld* World{Context->GetWorld()};
    UWorld::FDispatchScope DuplicateScope{World};
    ULevel* Level{World->GetPersistentLevel()};
    const ULevel* SourceLevel{SourceWorld->GetPersistentLevel()};
    Objects.Add(SourceWorld, World);
    Objects.Add(SourceLevel, Level);

    if (!CopyProperties(SourceWorld, World, Error) || !CopyProperties(SourceLevel, Level, Error)) {
        return nullptr;
    }

    for (const auto& SourceActor : SourceLevel->GetActors()) {
        if (SourceActor->IsPendingDestroy()) {
            continue;
        }

        auto Object{CreateObject(SourceActor.get(), &Objects, Error)};

        if (Object == nullptr) {
            return nullptr;
        }

        std::unique_ptr<AActor> Actor{static_cast<AActor*>(Object.release())};

        for (const auto& SourceComponent : SourceActor->GetComponents()) {
            auto ComponentObject{CreateObject(SourceComponent.get(), &Objects, Error)};

            if (ComponentObject == nullptr) {
                return nullptr;
            }

            std::unique_ptr<UActorComponent> Component{static_cast<UActorComponent*>(ComponentObject.release())};

            if (!Actor->AddComponent(std::move(Component))) {
                Error = "Cannot attach duplicated component";
                return nullptr;
            }
        }

        if (!Actor->SetRootComponent(Cast<USceneComponent>(Objects.FindDuplicate(SourceActor->GetRootComponent()))) || !Level->AddActor(std::move(Actor))) {
            Error = "Cannot attach duplicated actor";
            return nullptr;
        }
    }

    for (const auto& [Original, Duplicate] : Objects.GetObjects()) {
        if (!Duplicate->PostDuplicate(Original, &Objects)) {
            Error = "Duplicate fixup failed: " + Original->GetClass()->GetName();
            return nullptr;
        }

        if (!World->mPendingActors.empty() || World->mShutdownRequested || World->mEndPlayRequested) {
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
    FObjectDuplicationContext Objects{};
    TArray<std::unique_ptr<AActor>> Copies{};
    for (FObjectHandle Handle : Actors) {
        const auto* Source{Cast<AActor>(FObjectRegistry::Get()->Resolve(Handle))};
        if (Source == nullptr || Source->GetWorld() != World || Source->IsPendingDestroy() || Objects.FindDuplicate(Source) != nullptr) {
            Error = "Invalid source actor";
            return {};
        }
        auto Object{CreateObject(Source, &Objects, Error)};
        if (Object == nullptr) {
            return {};
        }
        std::unique_ptr<AActor> Copy{static_cast<AActor*>(Object.release())};
        for (const auto& Component : Source->GetComponents()) {
            auto Duplicate{CreateObject(Component.get(), &Objects, Error)};
            if (Duplicate == nullptr || !Copy->AddComponent(std::unique_ptr<UActorComponent>{static_cast<UActorComponent*>(Duplicate.release())})) {
                return {};
            }
        }
        if (Source->GetRootComponent() != nullptr && !Copy->SetRootComponent(Cast<USceneComponent>(Objects.FindDuplicate(Source->GetRootComponent())))) {
            return {};
        }
        Copies.push_back(std::move(Copy));
    }
    for (const auto& [Original, Duplicate] : Objects.GetObjects()) {
        if (!Duplicate->PostDuplicate(Original, &Objects)) {
            Error = "Actor reference fixup failed";
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
