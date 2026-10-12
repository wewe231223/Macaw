#include "pch.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Engine/Serialization/FSceneSerializer.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FJsonFile.h"
#include "World/UWorld.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Core/Console/Console.h"

#include "World/Component/UMeshComponent.h"
#include "World/Component/USubUVComponent.h"
#include <unordered_map>
#include <unordered_set>

namespace {
    bool MigrateUVComponents(rapidjson::Document& Document) {
        rapidjson::Document::AllocatorType& Allocator{Document.GetAllocator()};

        for (rapidjson::Value& Actor : Document["Actors"].GetArray()) {
            if (!Actor.IsObject() || !Actor.HasMember("Components") || !Actor["Components"].IsArray()) {
                return false;
            }

            rapidjson::Value Controllers{rapidjson::kArrayType};

            for (rapidjson::Value& Component : Actor["Components"].GetArray()) {
                if (!Component.IsObject() || !Component.HasMember("TypeName") || !Component["TypeName"].IsString()) {
                    return false;
                }

                const std::string_view Type{Component["TypeName"].GetString()};
                const bool FlipBook{Type == "USubUVComponent" && Component.HasMember("Transform")};
                const bool Scroll{Type == "UScrollUVComponent"};

                if (!FlipBook && !Scroll) {
                    continue;
                }

                if (!Component.HasMember("Guid") || !Component["Guid"].IsString()) {
                    return false;
                }

                rapidjson::Value Controller{rapidjson::kObjectType};
                const FString Guid{FGuid::NewGuid().ToString()};

                Controller.AddMember("TypeName", rapidjson::Value{FlipBook ? "UFlipBookComponent" : "UUVScrollComponent", Allocator}, Allocator);
                Controller.AddMember("Guid", rapidjson::Value{Guid.c_str(), Allocator}, Allocator);
                Controller.AddMember("GuidTargetComponent", rapidjson::Value{Component["Guid"], Allocator}, Allocator);

                const char* Fields[]{"SubImageHorizontal", "SubImageVertical", "TotalFrame", "FrameRate", "ScrollSpeed", "bLooping", "bPlaying", "bActive"};

                for (const char* Field : Fields) {
                    if (Component.HasMember(Field)) {
                        Controller.AddMember(rapidjson::Value{Field, Allocator}, rapidjson::Value{Component[Field], Allocator}, Allocator);
                    }
                }

                Controllers.PushBack(Controller, Allocator);
                Component["TypeName"].SetString("UBillboardComponent", Allocator);

                rapidjson::Value Minimum{rapidjson::kArrayType};
                rapidjson::Value Maximum{rapidjson::kArrayType};

                Minimum.PushBack(0.0f, Allocator).PushBack(0.0f, Allocator);
                Maximum.PushBack(1.0f, Allocator).PushBack(1.0f, Allocator);

                if (Component.HasMember("UVMin")) {
                    Component["UVMin"] = Minimum;
                } else {
                    Component.AddMember("UVMin", Minimum, Allocator);
                }

                if (Component.HasMember("UVMax")) {
                    Component["UVMax"] = Maximum;
                } else {
                    Component.AddMember("UVMax", Maximum, Allocator);
                }
            }

            for (rapidjson::Value& Controller : Controllers.GetArray()) {
                Actor["Components"].PushBack(Controller, Allocator);
            }
        }

        return true;
    }

    struct FSceneObject {
        const UClass* mType{};
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

    bool AddObject(const rapidjson::Value& Json, const UClass* BaseType, const FGuid& Owner, std::unordered_map<FGuid, FSceneObject>& Objects) {
        if (!Json.IsObject() || !Json.HasMember("TypeName") || !Json["TypeName"].IsString() || Json["TypeName"].GetStringLength() >= NameSize) {
            return false;
        }

        FGuid Guid{};
        const UClass* Type{FReflectionRegistry::FindClass(Json["TypeName"].GetString())};

        if (!ReadGuid(Json, "Guid", Guid, true) || Type == nullptr || !Type->CanCreateInstance() || !Type->IsChildOf(BaseType)) {
            return false;
        }

        return Objects.emplace(Guid, FSceneObject{Type, Owner, {}}).second;
    }

    bool ValidateReference(const rapidjson::Value& Json, const char* Name, const UClass* Type, const std::unordered_map<FGuid, FSceneObject>& Objects, FGuid& Guid) {
        if (!ReadGuid(Json, Name, Guid, false)) {
            return false;
        }

        if (!Guid.IsValid()) {
            return true;
        }

        const auto Found{Objects.find(Guid)};

        return Found != Objects.end() && Found->second.mType->IsChildOf(Type);
    }

    bool ValidateName(const rapidjson::Value& Json, const FGuid& Outer, TMap<FGuid, TSet<FName>>& Names) {
        const auto Member{Json.FindMember("Name")};

        if (Member == Json.MemberEnd()) {
            return true;
        }

        if (!Member->value.IsString() || Member->value.GetStringLength() >= NameSize) {
            return false;
        }

        const FName Name{std::string_view{Member->value.GetString(), Member->value.GetStringLength()}};

        return Name.IsNone() || (UObjectSystem::IsValidObjectName(Name) && Names[Outer].insert(Name).second);
    }

    bool ValidateScene(const rapidjson::Document& Document) {
        TMap<FGuid, TSet<FName>> Names{};

        std::unordered_map<FGuid, FSceneObject> Objects{};

        for (const rapidjson::Value& Actor : Document["Actors"].GetArray()) {
            if (!AddObject(Actor, AActor::StaticClass(), {}, Objects) || !ValidateName(Actor, {}, Names) || !Actor.HasMember("Components") || !Actor["Components"].IsArray()) {
                return false;
            }

            FGuid ActorGuid{};

            ReadGuid(Actor, "Guid", ActorGuid, true);

            for (const rapidjson::Value& Component : Actor["Components"].GetArray()) {
                if (!AddObject(Component, UActorComponent::StaticClass(), ActorGuid, Objects) || !ValidateName(Component, ActorGuid, Names)) {
                    return false;
                }
            }
        }

        for (const rapidjson::Value& Actor : Document["Actors"].GetArray()) {
            FGuid ActorGuid{};

            ReadGuid(Actor, "Guid", ActorGuid, true);

            FGuid RootGuid{};

            if (!ValidateReference(Actor, "GuidRootComponent", USceneComponent::StaticClass(), Objects, RootGuid) || (RootGuid.IsValid() && Objects.at(RootGuid).mOwner != ActorGuid)) {
                return false;
            }

            for (const rapidjson::Value& Component : Actor["Components"].GetArray()) {
                FGuid Guid{};

                ReadGuid(Component, "Guid", Guid, true);

                FGuid ParentGuid{};
                FGuid MeshGuid{};
                FGuid TargetGuid{};

                if (!ValidateReference(Component, "Parent", USceneComponent::StaticClass(), Objects, ParentGuid) || !ValidateReference(Component, "GuidMeshComponent", UMeshComponent::StaticClass(), Objects, MeshGuid)) {
                    return false;
                }

                Objects.at(Guid).mParent = ParentGuid;

                if (Objects.at(Guid).mType->IsChildOf(USubUVComponent::StaticClass()) && (!ValidateReference(Component, "GuidTargetComponent", UPrimitiveComponent::StaticClass(), Objects, TargetGuid) || (TargetGuid.IsValid() && Objects.at(TargetGuid).mOwner != ActorGuid))) {
                    return false;
                }
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

    void RemapConflictingGuids(rapidjson::Document& Document, const UWorld& World) {
        TSet<FGuid> ExistingGuids{};

        for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
            ExistingGuids.insert(Actor->GetGuid());

            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                ExistingGuids.insert(Component->GetGuid());
            }
        }

        TMap<FGuid, FGuid> RemappedGuids{};
        TSet<FGuid> SceneGuids{};

        for (const rapidjson::Value& Actor : Document["Actors"].GetArray()) {
            FGuid Guid{};

            ReadGuid(Actor, "Guid", Guid, true);
            SceneGuids.insert(Guid);

            for (const rapidjson::Value& Component : Actor["Components"].GetArray()) {
                ReadGuid(Component, "Guid", Guid, true);
                SceneGuids.insert(Guid);
            }
        }

        for (const FGuid& Guid : SceneGuids) {
            if (UObjectSystem::FindHandleByGuid(Guid).IsValid() && !ExistingGuids.contains(Guid)) {
                FGuid NewGuid{FGuid::NewGuid()};

                while (UObjectSystem::FindHandleByGuid(NewGuid).IsValid() || SceneGuids.contains(NewGuid) || std::ranges::any_of(RemappedGuids, [&NewGuid](const auto& Entry) {
                    return Entry.second == NewGuid;
                })) {
                    NewGuid = FGuid::NewGuid();
                }

                RemappedGuids.emplace(Guid, NewGuid);
            }
        }

        const auto RemapObject{[&Document, &RemappedGuids](rapidjson::Value& Object) {
            TArray<FString> Fields{FString{"Guid"}};
            const UClass* Class{FReflectionRegistry::FindClass(FName{Object["TypeName"].GetString()})};

            for (const FProperty* Property : Class->GetProperties()) {
                if (Property->HasAnyFlags(EPropertyFlags::Reference) && !Property->HasAnyFlags(EPropertyFlags::Transient) && Property->GetKind() == EPropertyKind::Object) {
                    Fields.push_back(Property->GetSerializationName());
                }
            }

            for (const FString& FieldName : Fields) {
                const char* Field{FieldName.c_str()};
                const auto Member{Object.FindMember(Field)};
                FGuid Guid{};

                if (Member != Object.MemberEnd() && ReadGuid(Object, Field, Guid, false)) {
                    const auto Iterator{RemappedGuids.find(Guid)};

                    if (Iterator != RemappedGuids.end()) {
                        const FString String{Iterator->second.ToString()};

                        Member->value.SetString(String.c_str(), static_cast<Uint32>(String.size()), Document.GetAllocator());
                    }
                }
            }
        }};

        for (rapidjson::Value& Actor : Document["Actors"].GetArray()) {
            RemapObject(Actor);

            for (rapidjson::Value& Component : Actor["Components"].GetArray()) {
                RemapObject(Component);
            }
        }
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

    if (!MigrateUVComponents(LoadDocument) || !ValidateScene(LoadDocument)) {
        return false;
    }

    RemapConflictingGuids(LoadDocument, World);

    TArray<std::unique_ptr<AActor>> LoadedActors{};

    LoadedActors.reserve(LoadDocument["Actors"].Size());

    for (rapidjson::Value& ActorJson : LoadDocument["Actors"].GetArray()) {
        const UClass* Type{FReflectionRegistry::FindClass(ActorJson["TypeName"].GetString())};
        std::unique_ptr<UObject> CreatedObject{Type->CreateInstance()};

        if (CreatedObject == nullptr || !CreatedObject->GetClass()->IsChildOf(AActor::StaticClass())) {
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
        if (!Actor->SetOuter(World.mPersistentLevel.get())) {
            World.ClearActors();
            return false;
        }
    }

    for (Uint32 Pass{}; Pass < 2; ++Pass) {
        for (const std::unique_ptr<AActor>& Actor : World.mPersistentLevel->mActors) {
            if (Actor->GetName().IsNone() == (Pass == 1) && !UObjectSystem::Register(Actor.get()).IsValid()) {
                World.ClearActors();
                return false;
            }
        }
    }

    for (const std::unique_ptr<AActor>& Actor : World.mPersistentLevel->mActors) {
        for (Uint32 Pass{}; Pass < 2; ++Pass) {
            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                if (Component->GetName().IsNone() == (Pass == 1) && !UObjectSystem::Register(Component.get()).IsValid()) {
                    World.ClearActors();
                    return false;
                }
            }
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
