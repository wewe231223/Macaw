#include "pch.h"
#include "Engine/Serialization/FSceneSerializer.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FJsonFile.h"
#include "World/UWorld.h"
#include "CoreUObject/TypeRegistry.h"
#include "Core/Console/Console.h"

#include "World/Component/UMeshComponent.h"
#include <unordered_map>
#include <unordered_set>

namespace {
    struct FSceneObject {
        const FTypeInfo* mType{};
        FGuid mOwner{};
        FGuid mParent{};
    };

    bool ReadGuid(const rapidjson::Value& Object, const char* Name, FGuid& Guid, bool Required) {
        const auto Member{Object.FindMember(Name)};

        if (Member == Object.MemberEnd()) {
            return !Required;
        }

        if (!Member->value.IsString()) {
            return false;
        }

        if (!Required && Member->value.GetStringLength() == 0) {
            return true;
        }

        return Guid.Parse(Member->value.GetString()) && (!Required || Guid.IsValid());
    }

    bool AddObject(const rapidjson::Value& Json, const FTypeInfo* BaseType, const FGuid& Owner, std::unordered_map<FGuid, FSceneObject>& Objects, const std::unordered_set<FGuid>& ExistingGuids) {
        if (!Json.IsObject() || !Json.HasMember("TypeName") || !Json["TypeName"].IsString()) {
            return false;
        }

        FGuid Guid{};
        const FTypeInfo* Type{TypeRegistry::Find(Json["TypeName"].GetString())};

        if (!ReadGuid(Json, "Guid", Guid, true) || Type == nullptr || Type->mCreator == nullptr || !Type->IsA(BaseType)) {
            return false;
        }

        if (UObjectSystem::FindHandleByGuid(Guid).IsValid() && !ExistingGuids.contains(Guid)) {
            return false;
        }

        return Objects.emplace(Guid, FSceneObject{Type, Owner, {}}).second;
    }

    bool ValidateReference(const rapidjson::Value& Json, const char* Name, const FTypeInfo* Type, const std::unordered_map<FGuid, FSceneObject>& Objects, FGuid& Guid) {
        if (!ReadGuid(Json, Name, Guid, false)) {
            return false;
        }

        if (!Guid.IsValid()) {
            return true;
        }

        const auto Found{Objects.find(Guid)};

        return Found != Objects.end() && Found->second.mType->IsA(Type);
    }

    bool ValidateScene(const rapidjson::Document& Document, const UWorld& World) {
        std::unordered_set<FGuid> ExistingGuids{};

        for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
            ExistingGuids.insert(Actor->GetGuid());

            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                ExistingGuids.insert(Component->GetGuid());
            }
        }

        std::unordered_map<FGuid, FSceneObject> Objects{};

        for (const rapidjson::Value& Actor : Document["Actors"].GetArray()) {
            if (!AddObject(Actor, AActor::StaticTypeInfo(), {}, Objects, ExistingGuids) || !Actor.HasMember("Components") || !Actor["Components"].IsArray()) {
                return false;
            }

            FGuid ActorGuid{};

            ReadGuid(Actor, "Guid", ActorGuid, true);

            for (const rapidjson::Value& Component : Actor["Components"].GetArray()) {
                if (!AddObject(Component, UActorComponent::StaticTypeInfo(), ActorGuid, Objects, ExistingGuids)) {
                    return false;
                }
            }
        }

        for (const rapidjson::Value& Actor : Document["Actors"].GetArray()) {
            FGuid ActorGuid{};

            ReadGuid(Actor, "Guid", ActorGuid, true);

            FGuid RootGuid{};

            if (!ValidateReference(Actor, "GuidRootComponent", USceneComponent::StaticTypeInfo(), Objects, RootGuid) || (RootGuid.IsValid() && Objects.at(RootGuid).mOwner != ActorGuid)) {
                return false;
            }

            for (const rapidjson::Value& Component : Actor["Components"].GetArray()) {
                FGuid Guid{};

                ReadGuid(Component, "Guid", Guid, true);

                FGuid ParentGuid{};
                FGuid MeshGuid{};
                FGuid TargetGuid{};

                if (!ValidateReference(Component, "Parent", USceneComponent::StaticTypeInfo(), Objects, ParentGuid) || !ValidateReference(Component, "GuidMeshComponent", UMeshComponent::StaticTypeInfo(), Objects, MeshGuid) || !ValidateReference(Component, "TargetActorGuid", AActor::StaticTypeInfo(), Objects, TargetGuid)) {
                    return false;
                }

                Objects.at(Guid).mParent = ParentGuid;
            }
        }

        std::unordered_set<FGuid> Resolved{};

        for (const auto& [Guid, Object] : Objects) {
            std::unordered_set<FGuid> Chain{};
            FGuid Current{Guid};

            while (Current.IsValid() && !Resolved.contains(Current)) {
                if (!Chain.insert(Current).second) {
                    return false;
                }

                Current = Objects.at(Current).mParent;
            }

            Resolved.insert(Chain.begin(), Chain.end());
        }

        return true;
    }
}

bool FSceneSerializer::Save(UWorld& World, const std::filesystem::path& ScenePath) {
    if (!World.mInitialized || World.mLoadingScene || World.mBTickingActors || World.mCleaningUp || World.mBeginningPlay || World.mEndingPlay || World.mActorDispatchDepth > 0 || World.mFlushingActors) {
        return false;
    }

    World.FlushPendingDestroyActors();

    TArray<AActor*> Actors{};

    for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
        if (Actor->HasFinishedSpawning() && !Actor->IsBeingDestroyed()) {
            Actors.push_back(Actor.get());
        }
    }

    rapidjson::Document Document{};

    Document.SetObject();

    rapidjson::Document::AllocatorType& Allocator{Document.GetAllocator()};

    FArchiveJson ArchiveSave{Document, Allocator};

    ArchiveSave.SetAssetResolver(World.GetAssetRegistry());

    Uint32 FormatVersion{3};

    ArchiveSave.Serialize("FormatVersion", FormatVersion);

    std::size_t ArraySize{Actors.size()};

    ArchiveSave.BeginArrayScope("Actors", ArraySize);

    for (std::size_t CurrentIndex{0}, EndIndex{Actors.size()}; CurrentIndex < EndIndex; ++CurrentIndex) {
        ArchiveSave.BeginObjectScope(std::to_string(CurrentIndex));
        Actors[CurrentIndex]->Save(ArchiveSave);
        ArchiveSave.EndObjectScope();
    }

    ArchiveSave.EndArrayScope();

    return FJsonFile::Save(ScenePath, Document);
}

bool FSceneSerializer::Load(UWorld& World, const std::filesystem::path& ScenePath) {
    if (!World.mInitialized || World.mLoadingScene || World.mBTickingActors || World.mCleaningUp || World.mBeginningPlay || World.mEndingPlay || World.mActorDispatchDepth > 0 || World.mFlushingActors) {
        return false;
    }

    const bool WasPlaying{World.mHasBegunPlay};

    World.EndPlay(EEndPlayReason::LevelTransition);
    World.mLoadingScene = true;

    const bool Loaded{LoadInternal(World, ScenePath)};

    World.mLoadingScene = false;
    World.RefreshActorTicks();

    if (WasPlaying) {
        World.BeginPlay();
    }

    return Loaded;
}

bool FSceneSerializer::LoadInternal(UWorld& World, const std::filesystem::path& ScenePath) {
    const auto FinishLoad{[&World]() {
        if (!World.GetPickingSubsystem().RebuildAccelerationStructure()) {
            Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Failed to rebuild the picking acceleration structure after loading the scene.");
        }

        World.MarkStructureDirty();

        return true;
    }};

    rapidjson::Document LoadDocument{};

    if (!FJsonFile::Load(ScenePath, LoadDocument)) {
        return false;
    }

    if (LoadDocument.HasParseError() || !LoadDocument.IsObject() || !LoadDocument.HasMember("FormatVersion") || !LoadDocument["FormatVersion"].IsUint() || LoadDocument["FormatVersion"].GetUint() != 3 || !LoadDocument.HasMember("Actors") || !LoadDocument["Actors"].IsArray()) {
        return false;
    }

    if (World.mAssetRegistry == nullptr) {
        return false;
    }

    if (!ValidateScene(LoadDocument, World)) {
        return false;
    }

    TArray<std::unique_ptr<AActor>> LoadedActors{};

    LoadedActors.reserve(LoadDocument["Actors"].Size());

    for (rapidjson::Value& ActorJson : LoadDocument["Actors"].GetArray()) {
        const FTypeInfo* Type{TypeRegistry::Find(ActorJson["TypeName"].GetString())};
        std::unique_ptr<UObject> CreatedObject{Type->mCreator()};

        if (CreatedObject == nullptr || !CreatedObject->GetTypeInfo()->IsA(AActor::StaticTypeInfo())) {
            return false;
        }

        std::unique_ptr<AActor> Actor{static_cast<AActor*>(CreatedObject.release())};
        FArchiveJson Archive{ActorJson};

        Archive.SetAssetResolver(World.mAssetRegistry);

        if (!Actor->PreLoadComponents(Archive, false)) {
            return false;
        }

        Actor->Load(Archive);

        if (Archive.HasError()) {
            return false;
        }

        LoadedActors.push_back(std::move(Actor));
    }

    World.ClearActors();
    World.mPersistentLevel->mActors = std::move(LoadedActors);

    for (const std::unique_ptr<AActor>& Actor : World.mPersistentLevel->mActors) {
        UObjectSystem::Register(Actor.get());

        for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
            UObjectSystem::Register(Component.get());
        }
    }

    for (const std::unique_ptr<AActor>& Actor : World.mPersistentLevel->mActors) {
        if (!Actor->ResolveLoadedReferences()) {
            World.ClearActors();
            return false;
        }
    }

    ++World.mActorDispatchDepth;

    for (std::size_t Index{}; Index < World.mPersistentLevel->mActors.size(); ++Index) {
        World.InitializeLoadedActor(*World.mPersistentLevel->mActors[Index]);
    }

    --World.mActorDispatchDepth;

    return FinishLoad();
}
