/**
 * @file FWorldSerializer.cpp
 * @brief 월드 액터와 컴포넌트의 직렬화 및 객체 참조 복원에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/FWorldSerializer.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Core/Archive.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/Component/UCameraComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/AActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldContext.h"
#include "CoreUObject/UObject.h"
#include "Core/GuidHash.h"
#include "CoreUObject/Reflection/UClass.h"

#include <utility>

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 월드의 활성 카메라와 입력 대상 참조를 저장하거나 검증하여 복원합니다.
     * @param[in,out] Archive 로드 또는 저장 모드와 오류 상태를 제공하는 아카이브입니다.
     * @param[in] World 조회·설정 또는 직렬화할 월드입니다.
     * @param[in,out] Objects 수집할 객체 목록 또는 GUID별 비소유 객체 대응표입니다.
     */
    void SerializeWorldReferences(FArchive& Archive, UWorld* World, TMap<FGuid, UObject*, FGuidHash>* Objects) {
        constexpr const char* Names[]{"ActiveCamera", "InputTarget"};
        for (Uint32 Index{}; Index < 2 && !Archive.HasError(); ++Index) {
            const char* Name{Names[Index]};
            if (Archive.IsLoading() && !Archive.HasField(Name)) {
                continue;
            }
            UObject* Object{Index == 0 ? static_cast<UObject*>(World->GetActiveCamera()) : World->GetInputTarget()};
            FString Reference{Object != nullptr ? Object->GetGuid().ToString() : FString{}};
            Archive.Serialize(Name, Reference);
            if (!Archive.IsLoading() || Archive.HasError()) {
                continue;
            }
            Object = nullptr;
            if (!Reference.empty()) {
                FGuid Guid{};
                const bool Parsed{FGuid::Parse(Reference, Guid) && Archive.RemapObjectReference(Guid)};
                const auto Position{Objects->find(Guid)};
                if (!Parsed || Position == Objects->end() || !World->ContainsObject(Position->second) || (Index == 0 ? Cast<UCameraComponent>(Position->second) == nullptr : Cast<AActor>(Position->second) == nullptr)) {
                    Archive.SetError(FString{"Invalid world reference: "} + Name);
                    return;
                }
                Object = Position->second;
            }
            if (!(Index == 0 ? World->SetActiveCamera(Cast<UCameraComponent>(Object)) : World->SetInputTarget(Cast<AActor>(Object)))) {
                Archive.SetError(FString{"Cannot restore world reference: "} + Name);
            }
        }
    }

    /**
     * @brief 객체의 클래스 이름과 식별 프로퍼티를 직렬화합니다.
     * @param[in,out] Archive 로드 또는 저장 모드를 가진 아카이브입니다.
     * @param[in,out] Object 대상 객체입니다.
     * @param[in,out] Objects 장면 내부 참조를 복원할 비소유 대응표입니다.
     */
    /**
     * @par 이전 복제 구현의 설명
     * @verbatim
     * @brief 객체 식별자와 Transient를 제외한 리플렉션 프로퍼티를 복사합니다.
     * @endverbatim
     */
    /**
     * @par 이전 복제 구현의 설명
     * @verbatim
     * @brief 객체 식별자와 Transient를 제외한 리플렉션 프로퍼티를 복사합니다.
     * @param[in] Source 읽을 원본 객체입니다.
     * @param[in,out] Destination 같은 클래스의 복제 대상입니다.
     * @param[out] Error 복사할 수 없는 프로퍼티의 진단입니다.
     * @return 대상 프로퍼티를 모두 복사하면 true입니다.
     * @endverbatim
     */
    void SerializeHeader(FArchive& Archive, UObject* Object, TMap<FGuid, UObject*, FGuidHash>* Objects) {
        FString ClassName{Object->GetClass()->GetName()};
        Archive.Serialize("Class", ClassName);

        if (ClassName != Object->GetClass()->GetName()) {
            Archive.SetError("Unexpected object class: " + ClassName);
            return;
        }

        Object->Serialize(Archive);

        if (!Archive.HasError() && !Objects->emplace(Object->GetGuid(), Object).second) {
            Archive.SetError("Duplicate object identity in scene");
        }
    }

    /**
     * @brief 저장된 클래스가 예상 타입을 상속하는지 검사하고 객체를 생성·등록합니다.
     * @param[in,out] Archive 로드 또는 저장 모드를 가진 아카이브입니다.
     * @param[in,out] Objects 이 장면에서 읽거나 저장한 객체의 비소유 대응표입니다.
     * @param[in] Expected Expected에 적용할 입력 값입니다.
     * @return 생성한 객체의 단독 소유권이며, 생성 또는 검증 실패 시 nullptr입니다.
     */
    /**
     * @par 이전 복제 구현의 설명
     * @verbatim
     * @brief 원본 클래스의 객체를 생성하고 등록한 뒤 프로퍼티를 복사합니다.
     * @endverbatim
     */
    /**
     * @par 이전 복제 구현의 설명
     * @verbatim
     * @brief 원본 클래스의 객체를 생성하고 등록한 뒤 프로퍼티를 복사합니다.
     * @param[in] Source 생성할 클래스와 복사 값을 제공하는 원본입니다.
     * @param[in,out] Duplication 원본과 새 객체의 대응 관계를 추가할 표입니다.
     * @param[out] Error 생성, 등록 또는 프로퍼티 복사의 실패 원인입니다.
     * @return 새 객체의 단독 소유권이며 실패하면 nullptr입니다.
     * @endverbatim
     */
    std::unique_ptr<UObject> CreateSerializedObject(FArchive& Archive, TMap<FGuid, UObject*, FGuidHash>* Objects, const UClass* Expected) {
        FString ClassName{};
        Archive.Serialize("Class", ClassName);
        if (ClassName == "ABillboardTextActor") {
            ClassName = "AActor";
        }
        const UClass* Class{FReflectionRegistry::FindClass(ClassName)};

        if (Archive.HasError() || Class == nullptr || !Class->IsChildOf(Expected) || !Class->CanCreate()) {
            Archive.SetError("Unsupported class: " + ClassName);

            return nullptr;
        }

        auto Object{Class->CreateInstance()};

        if (Object == nullptr || Object->GetClass() != Class) {
            Archive.SetError("Object creation failed: " + ClassName);

            return nullptr;
        }

        Object->Serialize(Archive);

        if (!Archive.HasError() && !Objects->emplace(Object->GetGuid(), Object.get()).second) {
            Archive.SetError("Duplicate object identity in scene");
        }

        return Object;
    }

    /**
     * @brief 액터의 컴포넌트 목록과 장면 내부 참조를 직렬화합니다.
     * @param[in,out] Archive 로드 또는 저장 모드를 가진 아카이브입니다.
     * @param[in,out] Actor 생성, 편집 또는 직렬화할 액터입니다.
     * @param[in,out] Objects 이 장면에서 읽거나 저장한 객체의 비소유 대응표입니다.
     */
    void SerializeComponents(FArchive& Archive, AActor* Actor, TMap<FGuid, UObject*, FGuidHash>* Objects) {
        Uint32 Count{static_cast<Uint32>(Actor->GetComponents().size())};

        if (!Archive.BeginArray("Components", Count)) {
            return;
        }

        for (Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            if (Archive.IsLoading()) {
                auto Object{CreateSerializedObject(Archive, Objects, UActorComponent::StaticClass())};

                if (Object != nullptr && !Archive.HasError()) {
                    std::unique_ptr<UActorComponent> Component{static_cast<UActorComponent*>(Object.release())};

                    if (!Actor->AddComponent(std::move(Component))) {
                        Archive.SetError("Component ownership is invalid");
                    }
                }
            } else {
                SerializeHeader(Archive, Actor->GetComponents()[Index].get(), Objects);
            }

            Archive.EndObject();
        }

        Archive.EndArray();

        if (!Archive.IsLoading() || Archive.HasField("RootComponent")) {
            FString Root{Actor->GetRootComponent() != nullptr ? Actor->GetRootComponent()->GetGuid().ToString() : FString{}};
            Archive.Serialize("RootComponent", Root);

            if (Archive.IsLoading() && !Archive.HasError()) {
                USceneComponent* Component{};

                if (!Root.empty()) {
                    FGuid Guid{};

                    const bool Parsed{FGuid::Parse(Root, Guid) && Archive.RemapObjectReference(Guid)};
                    const auto Position{Objects->find(Guid)};

                    if (!Parsed || Position == Objects->end() || (Component = Cast<USceneComponent>(Position->second)) == nullptr || Component->GetOwner() != Actor) {
                        Archive.SetError("Invalid root component reference");
                        return;
                    }
                }

                if (!Actor->SetRootComponent(Component)) {
                    Archive.SetError("Invalid root component ownership");
                }
            }
        }
    }

    /**
     * @brief 액터 배열과 컴포넌트를 직렬화하고 로드한 객체의 소유권을 모읍니다.
     * @param[in,out] Archive 로드 또는 저장 모드와 오류 상태를 제공하는 아카이브입니다.
     * @param[in] Sources 저장할 원본 액터의 비소유 주소 목록입니다.
     * @param[in,out] Actors 직렬화하거나 수집할 액터 목록입니다.
     * @param[in,out] Objects 수집할 객체 목록 또는 GUID별 비소유 객체 대응표입니다.
     */
    void SerializeActors(FArchive& Archive, const TArray<AActor*>& Sources, TArray<std::unique_ptr<AActor>>& Actors, TMap<FGuid, UObject*, FGuidHash>* Objects) {
        Uint32 Count{static_cast<Uint32>(Sources.size())};

        if (!Archive.BeginArray("Actors", Count)) {
            return;
        }

        for (Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            if (Archive.IsLoading()) {
                auto Object{CreateSerializedObject(Archive, Objects, AActor::StaticClass())};

                if (Object != nullptr && !Archive.HasError()) {
                    std::unique_ptr<AActor> Actor{static_cast<AActor*>(Object.release())};
                    SerializeComponents(Archive, Actor.get(), Objects);
                    Actors.push_back(std::move(Actor));
                }
            } else {
                AActor* Actor{Sources[Index]};
                SerializeHeader(Archive, Actor, Objects);
                SerializeComponents(Archive, Actor, Objects);
            }

            Archive.EndObject();
        }

        Archive.EndArray();
    }
}

void FWorldSerializer::Serialize(FArchive& Archive, const FWorldContext* Context) {
    UWorld* World{Context != nullptr ? Context->GetWorld() : nullptr};

    if (World == nullptr || (World->IsDispatching() && !Archive.IsDuplicating()) || World->GetPersistentLevel() == nullptr) {
        Archive.SetError("Cannot serialize an invalid world or a world during dispatch");
        return;
    }

    UWorld::FDispatchScope Scope{World};
    TMap<FGuid, UObject*, FGuidHash> Objects{};
    Uint32 Version{1};
    Archive.Serialize("Version", Version);

    if (Version != 1) {
        Archive.SetError("Unsupported scene version");
        return;
    }

    if (!Archive.BeginObject("World")) {
        return;
    }

    SerializeHeader(Archive, World, &Objects);
    ULevel* Level{World->GetPersistentLevel()};

    if (Archive.BeginObject("PersistentLevel")) {
        SerializeHeader(Archive, Level, &Objects);
        TArray<AActor*> Sources{};
        TArray<std::unique_ptr<AActor>> Actors{};

        if (!Archive.IsLoading()) {
            for (const auto& Actor : Level->GetActors()) {
                if (!Actor->IsPendingDestroy()) {
                    Sources.push_back(Actor.get());
                }
            }
        } else if (!Level->GetActors().empty()) {
            Archive.SetError("Cannot load a scene into a populated level");
        }

        SerializeActors(Archive, Sources, Actors, &Objects);

        if (Archive.IsLoading() && !Archive.HasError()) {
            for (auto& Actor : Actors) {
                if (!Level->AddActor(std::move(Actor))) {
                    Archive.SetError("Actor ownership is invalid");
                    break;
                }
            }
        }

        Archive.EndObject();
    }

    SerializeWorldReferences(Archive, World, &Objects);
    Archive.EndObject();
}

void FWorldSerializer::SaveActors(FArchive& Archive, const TArray<AActor*>& Actors) {
    if (Archive.IsLoading()) {
        Archive.SetError("Actor writer is not saving");
        return;
    }

    TMap<FGuid, UObject*, FGuidHash> Objects{};
    TArray<std::unique_ptr<AActor>> Loaded{};
    SerializeActors(Archive, Actors, Loaded, &Objects);
}

TArray<std::unique_ptr<AActor>> FWorldSerializer::LoadActors(FArchive& Archive) {
    if (!Archive.IsLoading()) {
        Archive.SetError("Actor reader is not loading");
        return {};
    }

    TMap<FGuid, UObject*, FGuidHash> Objects{};
    TArray<std::unique_ptr<AActor>> Actors{};
    const TArray<AActor*> Sources{};
    SerializeActors(Archive, Sources, Actors, &Objects);
    return Actors;
}
