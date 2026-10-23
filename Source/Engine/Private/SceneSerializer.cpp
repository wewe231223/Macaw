/**
 * @file SceneSerializer.cpp
 * @brief 월드, 액터와 컴포넌트의 장면 파일 직렬화를 제공합니다.
 * @ingroup Engine
 */

#include "Engine/SceneSerializer.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Core/Archive.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/AActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldContext.h"
#include "CoreUObject/UObject.h"
#include "Core/GuidHash.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Serialization/JsonArchive.h"

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 객체의 클래스 이름과 식별 프로퍼티를 직렬화합니다.
     * @param[in,out] Archive 로드 또는 저장 모드를 가진 아카이브입니다.
     * @param[in,out] Object 대상 객체입니다.
     * @param[in,out] Objects 장면 내부 참조를 복원할 비소유 대응표입니다.
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
    std::unique_ptr<UObject> CreateSerializedObject(FArchive& Archive, TMap<FGuid, UObject*, FGuidHash>* Objects, const UClass* Expected) {
        FString ClassName{};
        Archive.Serialize("Class", ClassName);
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

                    const bool Parsed{FGuid::Parse(Root, Guid)};
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
}

/**
 * @brief 월드 컨텍스트를 JSON 장면 파일로 저장합니다.
 */
bool FSceneSerializer::Save(const std::filesystem::path& Path, FWorldContext* Context, FString& Error) {
    FJsonArchive Archive{EArchiveMode::Saving};
    Serialize(Archive, Context);

    const bool Saved{!Archive.HasError() && Archive.Save(Path)};
    Error = Archive.GetError();

    return Saved;
}

/**
 * @brief 파일을 읽고 검증한 새 월드 컨텍스트를 반환합니다.
 */
std::unique_ptr<FWorldContext> FSceneSerializer::Load(const std::filesystem::path& Path, FString& Error) {
    FJsonArchive Archive{EArchiveMode::Loading};

    if (!Archive.Load(Path)) {
        Error = Archive.GetError();

        return nullptr;
    }

    auto Context{std::make_unique<FWorldContext>()};
    Serialize(Archive, Context.get());
    Error = Archive.GetError();

    return Archive.HasError() ? nullptr : std::move(Context);
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FSceneSerializer::Serialize(FArchive& Archive, FWorldContext* Context) {
    if (Context == nullptr || Context->GetWorld() == nullptr || Context->GetWorld()->IsDispatching()) {
        Archive.SetError("Cannot serialize a world during dispatch");
        return;
    }

    TMap<FGuid, UObject*, FGuidHash> Objects{};
    Uint32 Version{1};
    Archive.Serialize("Version", Version);

    if (Version != 1) {
        Archive.SetError("Unsupported scene version");
    }

    UWorld* World{Context->GetWorld()};

    if (World == nullptr || !Archive.BeginObject("World")) {
        return;
    }

    SerializeHeader(Archive, World, &Objects);
    ULevel* Level{World->GetPersistentLevel()};

    if (Level != nullptr && Archive.BeginObject("PersistentLevel")) {
        SerializeHeader(Archive, Level, &Objects);
        Uint32 Count{static_cast<Uint32>(Level->GetActors().size())};

        if (Archive.BeginArray("Actors", Count)) {
            for (Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
                if (!Archive.BeginObject(std::to_string(Index))) {
                    break;
                }

                if (Archive.IsLoading()) {
                    auto Object{CreateSerializedObject(Archive, &Objects, AActor::StaticClass())};

                    if (Object != nullptr && !Archive.HasError()) {
                        std::unique_ptr<AActor> Actor{static_cast<AActor*>(Object.release())};
                        SerializeComponents(Archive, Actor.get(), &Objects);

                        if (!Archive.HasError() && !Level->AddActor(std::move(Actor))) {
                            Archive.SetError("Actor ownership is invalid");
                        }
                    }
                } else {
                    AActor* Actor{Level->GetActors()[Index].get()};

                    SerializeHeader(Archive, Actor, &Objects);

                    SerializeComponents(Archive, Actor, &Objects);
                }

                Archive.EndObject();
            }

            Archive.EndArray();
        }

        Archive.EndObject();
    }

    Archive.EndObject();
}
