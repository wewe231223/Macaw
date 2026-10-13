#include "pch.h"
#include "Engine/Serialization/FWorldSerializer.h"
#include "Core/Archive/FArchive.h"
#include "Core/Console/Console.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/UClass.h"
#include "World/UWorld.h"

FWorldSerializer::FLoadScope::FLoadScope(UWorld& World)
	: mWorld{World},
	  mWasPlaying{World.mHasBegunPlay} {
    mWorld.EndPlay(EEndPlayReason::LevelTransition);
    mWorld.mLoadingScene = true;
}

FWorldSerializer::FLoadScope::~FLoadScope() {
    mWorld.mLoadingScene = false;
    mWorld.RefreshActorTicks();

    if (mWasPlaying) {
        mWorld.BeginPlay();
    }
}

bool FWorldSerializer::CanSerialize(const UWorld& World) {
    return World.IsInitialized() && !World.IsBusy();
}

FWorldSerializer::FObjectGuidScope::FObjectGuidScope(FArchive& Archive, const TMap<FGuid, FGuid>* Remapping)
	: mArchive{Archive},
	  mPreviousRemapping{Archive.GetObjectGuidRemapping()} {
    mArchive.SetObjectGuidRemapping(Remapping);
}

FWorldSerializer::FObjectGuidScope::~FObjectGuidScope() {
    mArchive.SetObjectGuidRemapping(mPreviousRemapping);
}

bool FWorldSerializer::Save(UWorld& World, FArchive& Archive) {
    if (!CanSerialize(World) || !Archive.IsSaving() || Archive.HasError()) {
        return false;
    }

    World.FlushPendingDestroyActors();

    TArray<AActor*> Actors{};
    TArray<FActorRecord> Records{};

    for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
        if (!Actor->HasFinishedSpawning() || Actor->IsBeingDestroyed()) {
            continue;
        }

        Actors.push_back(Actor.get());

        if (!Archive.SupportsNamedFields()) {
            FActorRecord Record{{Actor->GetGuid(), Actor->GetClass()->GetClassName()}, {}};

            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                Record.mComponents.push_back({Component->GetGuid(), Component->GetClass()->GetClassName()});
            }

            Records.push_back(std::move(Record));
        }
    }

    Archive.SetAssetResolver(World.GetAssetRegistry());

    Uint32 FormatVersion{3};

    Archive.Serialize("FormatVersion", FormatVersion);

    if (!Archive.SupportsNamedFields() && !SerializeObjectTable(Archive, Records)) {
        return false;
    }

    std::size_t ArraySize{Actors.size()};

    Archive.BeginArrayScope("Actors", ArraySize);

    for (std::size_t Index{}; Index < Actors.size() && !Archive.HasError(); ++Index) {
        Archive.BeginObjectScope(std::to_string(Index));
        Actors[Index]->Save(Archive);
        Archive.EndObjectScope();
    }

    Archive.EndArrayScope();

    return !Archive.HasError();
}

bool FWorldSerializer::Load(UWorld& World, FArchive& Archive) {
    if (!CanSerialize(World) || !Archive.IsLoading() || Archive.HasError()) {
        return false;
    }

    const FLoadScope LoadScope{World};

    return LoadInternal(World, Archive);
}

bool FWorldSerializer::SerializeObjectRecord(FArchive& Archive, FObjectRecord& Record) {
    if (Archive.IsLoading() && Archive.SupportsNamedFields() && (!Archive.HasField("Guid") || !Archive.HasField("TypeName"))) {
        return false;
    }

    FString TypeName{Record.mTypeName.ToString()};

    Archive.Serialize("Guid", Record.mGuid);
    Archive.Serialize("TypeName", TypeName);

    if (Archive.HasError() || !Record.mGuid.IsValid() || TypeName.empty() || TypeName.size() >= NameSize) {
        return false;
    }

    if (Archive.IsLoading()) {
        Record.mTypeName = FName{TypeName};
    }

    return true;
}

bool FWorldSerializer::SerializeObjectTable(FArchive& Archive, TArray<FActorRecord>& Records) {
    std::size_t ActorCount{Records.size()};

    Archive.BeginArrayScope(Archive.SupportsNamedFields() ? "Actors" : "ObjectTable", ActorCount);

    if (Archive.IsLoading() && !Archive.HasError()) {
        Records.clear();
    }

    bool Valid{!Archive.HasError()};

    for (std::size_t Index{}; Index < ActorCount && Valid; ++Index) {
        if (Archive.IsLoading()) {
            Records.emplace_back();
        }

        FActorRecord& Record{Records[Index]};

        Archive.BeginObjectScope(std::to_string(Index));
        Valid = SerializeObjectRecord(Archive, Record.mObject);
        Valid = Valid && (!Archive.SupportsNamedFields() || Archive.HasField("Components"));

        std::size_t ComponentCount{Record.mComponents.size()};

        Archive.BeginArrayScope("Components", ComponentCount);

        if (Archive.IsLoading() && !Archive.HasError()) {
            Record.mComponents.clear();
        }

        Valid = Valid && !Archive.HasError();

        for (std::size_t ComponentIndex{}; ComponentIndex < ComponentCount && Valid; ++ComponentIndex) {
            if (Archive.IsLoading()) {
                Record.mComponents.emplace_back();
            }

            Archive.BeginObjectScope(std::to_string(ComponentIndex));
            Valid = SerializeObjectRecord(Archive, Record.mComponents[ComponentIndex]);
            Archive.EndObjectScope();
        }

        Archive.EndArrayScope();
        Archive.EndObjectScope();
    }

    Archive.EndArrayScope();

    return Valid && !Archive.HasError();
}

bool FWorldSerializer::ValidateObjectTable(const UWorld& World, const TArray<FActorRecord>& Records, bool AllowExistingGuids) {
    TSet<FGuid> ExistingGuids{};
    TSet<FGuid> Guids{};

    for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
        ExistingGuids.insert(Actor->GetGuid());

        for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
            ExistingGuids.insert(Component->GetGuid());
        }
    }

    const auto ValidateObject{[&ExistingGuids, &Guids, AllowExistingGuids](const FObjectRecord& Record, const UClass* BaseType) {
        const UClass* Type{FReflectionRegistry::FindClass(Record.mTypeName)};

        return Record.mGuid.IsValid() && Guids.insert(Record.mGuid).second && Type != nullptr && Type->CanCreateInstance() && Type->IsChildOf(BaseType) && (AllowExistingGuids || !UObjectSystem::FindHandleByGuid(Record.mGuid).IsValid() || ExistingGuids.contains(Record.mGuid));
    }};

    for (const FActorRecord& Record : Records) {
        if (!ValidateObject(Record.mObject, AActor::StaticClass())) {
            return false;
        }

        for (const FObjectRecord& Component : Record.mComponents) {
            if (!ValidateObject(Component, UActorComponent::StaticClass())) {
                return false;
            }
        }
    }

    return true;
}

bool FWorldSerializer::RemapObjectTable(const UWorld& SourceWorld, const UWorld& World, TArray<FActorRecord>& Records, TMap<FGuid, FGuid>& Remapping) {
    TSet<FGuid> UsedGuids{SourceWorld.GetGuid(), SourceWorld.GetPersistentLevel().GetGuid(), World.GetGuid(), World.GetPersistentLevel().GetGuid()};

    Remapping.emplace(SourceWorld.GetGuid(), World.GetGuid());
    Remapping.emplace(SourceWorld.GetPersistentLevel().GetGuid(), World.GetPersistentLevel().GetGuid());

    for (const FActorRecord& Record : Records) {
        UsedGuids.insert(Record.mObject.mGuid);

        for (const FObjectRecord& Component : Record.mComponents) {
            UsedGuids.insert(Component.mGuid);
        }
    }

    const auto RemapObject{[&UsedGuids, &Remapping](FObjectRecord& Record) {
        for (Uint32 Attempt{}; Attempt < 128; ++Attempt) {
            const FGuid Guid{FGuid::NewGuid()};

            if (!Guid.IsValid()) {
                return false;
            }

            if (UsedGuids.contains(Guid) || UObjectSystem::FindHandleByGuid(Guid).IsValid()) {
                continue;
            }

            if (!Remapping.emplace(Record.mGuid, Guid).second) {
                return false;
            }

            UsedGuids.insert(Guid);
            Record.mGuid = Guid;

            return true;
        }

        return false;
    }};

    for (FActorRecord& Record : Records) {
        if (!RemapObject(Record.mObject)) {
            return false;
        }

        for (FObjectRecord& Component : Record.mComponents) {
            if (!RemapObject(Component)) {
                return false;
            }
        }
    }

    return true;
}

std::unique_ptr<AActor> FWorldSerializer::CreateActor(const FActorRecord& Record) {
    const UClass* Type{FReflectionRegistry::FindClass(Record.mObject.mTypeName)};
    std::unique_ptr<UObject> CreatedObject{Type->CreateInstance()};

    if (CreatedObject == nullptr || !CreatedObject->IsA(AActor::StaticClass())) {
        return {};
    }

    std::unique_ptr<AActor> Actor{static_cast<AActor*>(CreatedObject.release())};

    for (const std::unique_ptr<UActorComponent>& Component : Actor->mComponents) {
        Component->UnregisterComponent();
        UObjectSystem::Unregister(Component.get(), Component->GetHandle());
    }

    Actor->mRootComponent = nullptr;
    Actor->mComponents.clear();
    Actor->mComponents.reserve(Record.mComponents.size());

    for (const FObjectRecord& ComponentRecord : Record.mComponents) {
        const UClass* ComponentType{FReflectionRegistry::FindClass(ComponentRecord.mTypeName)};
        std::unique_ptr<UObject> CreatedComponent{ComponentType->CreateInstance()};

        if (CreatedComponent == nullptr || !CreatedComponent->IsA(UActorComponent::StaticClass())) {
            return {};
        }

        std::unique_ptr<UActorComponent> Component{static_cast<UActorComponent*>(CreatedComponent.release())};

        Component->SetOwner(Actor.get());

        if (!Component->RestoreGuid(ComponentRecord.mGuid)) {
            return {};
        }

        Actor->mComponents.push_back(std::move(Component));
    }

    return Actor;
}

bool FWorldSerializer::ValidateLoadedActors(const TArray<std::unique_ptr<AActor>>& Actors, const TArray<FActorRecord>& Records) {
    TSet<FName> ActorNames{};

    for (std::size_t Index{}; Index < Actors.size(); ++Index) {
        const AActor& Actor{*Actors[Index]};
        const FActorRecord& Record{Records[Index]};

        if (Actor.GetComponents().size() != Record.mComponents.size() || Actor.GetGuid() != Record.mObject.mGuid || Actor.GetClass()->GetClassName() != Record.mObject.mTypeName || (!Actor.GetName().IsNone() && (!UObjectSystem::IsValidObjectName(Actor.GetName()) || !ActorNames.insert(Actor.GetName()).second))) {
            return false;
        }

        TSet<FName> ComponentNames{};

        for (std::size_t ComponentIndex{}; ComponentIndex < Actor.GetComponents().size(); ++ComponentIndex) {
            const UActorComponent& Component{*Actor.GetComponents()[ComponentIndex]};
            const FObjectRecord& ComponentRecord{Record.mComponents[ComponentIndex]};

            if (Component.GetGuid() != ComponentRecord.mGuid || Component.GetClass()->GetClassName() != ComponentRecord.mTypeName || (!Component.GetName().IsNone() && (!UObjectSystem::IsValidObjectName(Component.GetName()) || !ComponentNames.insert(Component.GetName()).second))) {
                return false;
            }
        }
    }

    return true;
}

bool FWorldSerializer::LoadInternal(UWorld& World, FArchive& Archive, const UWorld* SourceWorld) {
    if (!Archive.IsLoading() || Archive.HasError() || World.mAssetRegistry == nullptr) {
        return false;
    }

    if (Archive.SupportsNamedFields() && (!Archive.HasField("FormatVersion") || !Archive.HasField("Actors"))) {
        return false;
    }

    Archive.SetAssetResolver(World.mAssetRegistry);

    Uint32 FormatVersion{};

    Archive.Serialize("FormatVersion", FormatVersion);

    if (Archive.HasError() || FormatVersion != 3) {
        return false;
    }

    TArray<FActorRecord> Records{};

    if (!SerializeObjectTable(Archive, Records) || !ValidateObjectTable(World, Records, SourceWorld != nullptr)) {
        return false;
    }

    TMap<FGuid, FGuid> Remapping{};

    if (SourceWorld != nullptr && (!RemapObjectTable(*SourceWorld, World, Records, Remapping) || !ValidateObjectTable(World, Records))) {
        return false;
    }

    const FObjectGuidScope GuidScope{Archive, SourceWorld != nullptr ? &Remapping : Archive.GetObjectGuidRemapping()};

    TArray<std::unique_ptr<AActor>> LoadedActors{};
    std::size_t ActorCount{};

    Archive.BeginArrayScope("Actors", ActorCount);

    bool Valid{!Archive.HasError() && ActorCount == Records.size()};

    LoadedActors.reserve(Valid ? ActorCount : 0);

    for (std::size_t Index{}; Index < ActorCount && Valid; ++Index) {
        std::unique_ptr<AActor> Actor{CreateActor(Records[Index])};

        if (Actor == nullptr) {
            Valid = false;
            break;
        }

        Archive.BeginObjectScope(std::to_string(Index));

        Actor->Load(Archive);
        Valid = !Archive.HasError();

        Archive.EndObjectScope();
        LoadedActors.push_back(std::move(Actor));
    }

    Archive.EndArrayScope();

    return Valid && ValidateLoadedActors(LoadedActors, Records) && InstallActors(World, std::move(LoadedActors), SourceWorld != nullptr);
}

bool FWorldSerializer::InstallActors(UWorld& World, TArray<std::unique_ptr<AActor>> Actors, bool IsCloning) {
    World.ClearActors();
    World.mPersistentLevel->mActors = std::move(Actors);

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

    if (IsCloning) {
        for (const std::unique_ptr<AActor>& Actor : World.mPersistentLevel->mActors) {
            Actor->mWorld = &World;
        }
    }

    for (const std::unique_ptr<AActor>& Actor : World.mPersistentLevel->mActors) {
        if (!Actor->ResolveLoadedReferences()) {
            World.ClearActors();
            return false;
        }
    }

    {
        const UWorld::FActorDispatchScope Dispatch{&World};

        for (std::size_t Index{}; Index < World.mPersistentLevel->mActors.size(); ++Index) {
            if (IsCloning && !World.mPersistentLevel->mActors[Index]->IsBeingDestroyed()) {
                World.mPersistentLevel->mActors[Index]->OnAddedToWorld();
            }

            World.InitializeLoadedActor(*World.mPersistentLevel->mActors[Index]);
        }
    }

    if (!World.GetPickingSubsystem().RebuildAccelerationStructure()) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Failed to rebuild the picking acceleration structure after loading the scene.");
    }

    World.MarkStructureDirty();

    return true;
}
