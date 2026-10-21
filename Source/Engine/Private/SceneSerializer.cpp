#include "Engine/SceneSerializer.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Core/Archive.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/AActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldContext.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Serialization/JsonArchive.h"

namespace {
    void SerializeHeader(FArchive& Archive, UObject& Object) {
        FString ClassName{Object.GetClass()->GetName()};
        Archive.Serialize("Class", ClassName);

        if (ClassName != Object.GetClass()->GetName()) {
            Archive.SetError("Unexpected object class: " + ClassName);
            return;
        }

        Object.Serialize(Archive);
    }

    std::unique_ptr<UObject> CreateSerializedObject(FArchive& Archive, FObjectRegistry& Registry, const UClass* Expected) {
        FString ClassName{};
        Archive.Serialize("Class", ClassName);
        const UClass* Class{FReflectionRegistry::FindClass(ClassName)};

        if (Archive.HasError() || Class == nullptr || !Class->IsChildOf(Expected) || !Class->CanCreate()) {
            Archive.SetError("Unsupported class: " + ClassName);

            return nullptr;
        }

        auto Object{Class->CreateInstance()};

        if (Object == nullptr || Object->GetClass() != Class || !Registry.Register(*Object)) {
            Archive.SetError("Object creation failed: " + ClassName);

            return nullptr;
        }

        Object->Serialize(Archive);

        return Object;
    }

    void SerializeComponents(FArchive& Archive, AActor& Actor, FObjectRegistry& Registry) {
        Uint32 Count{static_cast<Uint32>(Actor.GetComponents().size())};

        if (!Archive.BeginArray("Components", Count)) {
            return;
        }

        if (Count != 1) {
            Archive.SetError("Week 1 actors require exactly one root component");
        }

        for (Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            if (Archive.IsLoading()) {
                auto Object{CreateSerializedObject(Archive, Registry, UStaticMeshComponent::StaticClass())};

                if (Object != nullptr && !Archive.HasError()) {
                    std::unique_ptr<UActorComponent> Component{static_cast<UActorComponent*>(Object.release())};

                    if (!Actor.AddComponent(std::move(Component))) {
                        Archive.SetError("Component ownership is invalid");
                    }
                }
            } else {
                SerializeHeader(Archive, *Actor.GetComponents()[Index]);
            }

            Archive.EndObject();
        }

        Archive.EndArray();
    }
}

bool FSceneSerializer::Save(const std::filesystem::path& Path, FWorldContext& Context, FString& Error) {
    FJsonArchive Archive{EArchiveMode::Saving};
    Serialize(Archive, Context);

    const bool Saved{!Archive.HasError() && Archive.Save(Path)};
    Error = Archive.GetError();

    return Saved;
}

std::unique_ptr<FWorldContext> FSceneSerializer::Load(const std::filesystem::path& Path, FString& Error) {
    FJsonArchive Archive{EArchiveMode::Loading};

    if (!Archive.Load(Path)) {
        Error = Archive.GetError();

        return nullptr;
    }

    auto Context{std::make_unique<FWorldContext>()};
    Serialize(Archive, *Context);
    Error = Archive.GetError();

    return Archive.HasError() ? nullptr : std::move(Context);
}

void FSceneSerializer::Serialize(FArchive& Archive, FWorldContext& Context) {
    Uint32 Version{1};
    Archive.Serialize("Version", Version);

    if (Version != 1) {
        Archive.SetError("Unsupported scene version");
    }

    UWorld* World{Context.GetWorld()};

    if (World == nullptr || !Archive.BeginObject("World")) {
        return;
    }

    SerializeHeader(Archive, *World);
    ULevel* Level{World->GetPersistentLevel()};

    if (Level != nullptr && Archive.BeginObject("PersistentLevel")) {
        SerializeHeader(Archive, *Level);
        Uint32 Count{static_cast<Uint32>(Level->GetActors().size())};

        if (Archive.BeginArray("Actors", Count)) {
            for (Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
                if (!Archive.BeginObject(std::to_string(Index))) {
                    break;
                }

                if (Archive.IsLoading()) {
                    auto Object{CreateSerializedObject(Archive, Context.GetRegistry(), AStaticMeshActor::StaticClass())};

                    if (Object != nullptr && !Archive.HasError()) {
                        std::unique_ptr<AActor> Actor{static_cast<AActor*>(Object.release())};
                        SerializeComponents(Archive, *Actor, Context.GetRegistry());

                        if (!Archive.HasError() && !Level->AddActor(std::move(Actor))) {
                            Archive.SetError("Actor ownership is invalid");
                        }
                    }
                } else {
                    AActor& Actor{*Level->GetActors()[Index]};

                    SerializeHeader(Archive, Actor);

                    SerializeComponents(Archive, Actor, Context.GetRegistry());
                }

                Archive.EndObject();
            }

            Archive.EndArray();
        }

        Archive.EndObject();
    }

    Archive.EndObject();
}
