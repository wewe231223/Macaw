#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Engine/Serialization/FWorldSerializer.h"
#include "Core/Archive/FArchive.h"
#include "Core/Console/Console.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/UClass.h"
#include "World/UWorld.h"
#include "Serialization/FArchiveMemory.h"

FWorldSerializer::FLoadScope::FLoadScope(UWorld& World)
	: mWorld{World},
	  mWasPlaying{World.HasBegunPlay()} {
    mWorld.EndPlay(EEndPlayReason::LevelTransition);
    mWorld.mState.Set(UWorld::EStateFlag::LoadingScene);
}

FWorldSerializer::FLoadScope::~FLoadScope() {
    mWorld.mState.Clear(UWorld::EStateFlag::LoadingScene);
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
                Record.mComponents.push_back({Component->GetGuid(), Component->GetClass()->GetClassName(), Component->GetDefaultComponentName()});
            }

            Records.push_back(std::move(Record));
        }
    }

    Archive.SetAssetResolver(FAssetRegistry::TryGet());

    Uint32 FormatVersion{4};

    Archive.Serialize("FormatVersion", FormatVersion);

    if (!Archive.SupportsNamedFields() && !SerializeObjectTable(Archive, Records, FormatVersion)) {
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

bool FWorldSerializer::SerializeObjectRecord(FArchive& Archive, FObjectRecord& Record, Uint32 FormatVersion) {
    if (Archive.IsLoading() && Archive.SupportsNamedFields() && (!Archive.HasField("Guid") || !Archive.HasField("TypeName"))) {
        return false;
    }

    FString TypeName{Record.mTypeName.ToString()};

    Archive.Serialize("Guid", Record.mGuid);
    Archive.Serialize("TypeName", TypeName);

    if ((!Archive.SupportsNamedFields() && FormatVersion >= 4) || (Archive.SupportsNamedFields() && Archive.HasField("DefaultComponentName"))) {
        Archive.Serialize("DefaultComponentName", Record.mDefaultComponentName);
    }

    if (!Record.mDefaultComponentName.IsNone() && !UObjectSystem::IsValidObjectName(Record.mDefaultComponentName)) {
        return false;
    }

    if (Archive.HasError() || !Record.mGuid.IsValid() || TypeName.empty() || TypeName.size() >= NameSize) {
        return false;
    }

    if (Archive.IsLoading()) {
        Record.mTypeName = FName{TypeName};
    }

    return true;
}

bool FWorldSerializer::SerializeObjectTable(FArchive& Archive, TArray<FActorRecord>& Records, Uint32 FormatVersion) {
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
        Valid = SerializeObjectRecord(Archive, Record.mObject, FormatVersion);
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
            Valid = SerializeObjectRecord(Archive, Record.mComponents[ComponentIndex], FormatVersion);
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

std::unique_ptr<AActor> FWorldSerializer::CreateAndLoadActor(const FActorRecord& Record, FArchive& Archive) {
    const UClass* Type{FReflectionRegistry::FindClass(Record.mObject.mTypeName)};
    std::unique_ptr<UObject> CreatedObject{Type->CreateInstance()};

    if (CreatedObject == nullptr || !CreatedObject->IsA(AActor::StaticClass())) {
        return {};
    }

    std::unique_ptr<AActor> Actor{static_cast<AActor*>(CreatedObject.release())};

    TArray<std::pair<USceneComponent*, USceneComponent*>> DefaultAttachments{};

    for (const std::unique_ptr<UActorComponent>& Component : Actor->mComponents) {
        if (Component->IsA(USceneComponent::StaticClass())) {
            USceneComponent* Scene{static_cast<USceneComponent*>(Component.get())};

            if (USceneComponent* Parent{Scene->GetParent()}) {
                DefaultAttachments.emplace_back(Scene, Parent);
            }
        }
    }

    for (const auto& [Child, Parent] : DefaultAttachments) {
        Child->DetachFromComponent();
    }

    std::vector<std::unique_ptr<UActorComponent>> Defaults{std::move(Actor->mComponents)};

    Actor->mComponents.clear();
    Actor->mComponents.reserve(Record.mComponents.size() + Defaults.size());

    for (const std::unique_ptr<UActorComponent>& Component : Defaults) {
        UObjectSystem::Unregister(Component.get(), Component->GetHandle());
    }

    for (const FObjectRecord& ComponentRecord : Record.mComponents) {
        const UClass* ComponentType{FReflectionRegistry::FindClass(ComponentRecord.mTypeName)};
        std::unique_ptr<UActorComponent> Component{};

        if (!ComponentRecord.mDefaultComponentName.IsNone()) {
            const auto Found{std::ranges::find_if(Defaults, [&ComponentRecord](const std::unique_ptr<UActorComponent>& Candidate) {
                return Candidate != nullptr && Candidate->GetDefaultComponentName() == ComponentRecord.mDefaultComponentName;
            })};

            if (Found == Defaults.end() || (*Found)->GetClass() != ComponentType) {
                return {};
            }

            Component = std::move(*Found);
        } else {
            std::unique_ptr<UObject> CreatedComponent{ComponentType->CreateInstance()};

            if (CreatedComponent == nullptr || !CreatedComponent->IsA(UActorComponent::StaticClass())) {
                return {};
            }

            Component.reset(static_cast<UActorComponent*>(CreatedComponent.release()));
            Component->SetOwner(Actor.get());
        }

        if (!Component->RestoreGuid(ComponentRecord.mGuid)) {
            return {};
        }

        Actor->mComponents.push_back(std::move(Component));
    }

    for (const auto& [Child, Parent] : DefaultAttachments) {
        Child->mPendingParentGuid = Parent->GetGuid();
    }

    Actor->Load(Archive);

    if (Archive.HasError()) {
        return {};
    }

    for (std::unique_ptr<UActorComponent>& Component : Defaults) {
        if (Component != nullptr) {
            Actor->mComponents.push_back(std::move(Component));
        }
    }

    return Actor;
}

bool FWorldSerializer::ValidateLoadedActors(const TArray<std::unique_ptr<AActor>>& Actors, const TArray<FActorRecord>& Records) {
    if (Actors.size() != Records.size()) {
        return false;
    }

    TSet<FName> ActorNames{};

    for (std::size_t Index{}; Index < Actors.size(); ++Index) {
        const AActor& Actor{*Actors[Index]};
        const FActorRecord& Record{Records[Index]};

        if (Actor.GetGuid() != Record.mObject.mGuid || Actor.GetClass()->GetClassName() != Record.mObject.mTypeName || (!Actor.GetName().IsNone() && (!UObjectSystem::IsValidObjectName(Actor.GetName()) || !ActorNames.insert(Actor.GetName()).second))) {
            return false;
        }

        TSet<FName> ComponentNames{};
        TSet<FName> DefaultNames{};
        TSet<FGuid> MatchedGuids{};

        for (const std::unique_ptr<UActorComponent>& Component : Actor.GetComponents()) {
            const auto Found{std::ranges::find(Record.mComponents, Component->GetGuid(), &FObjectRecord::mGuid)};

            if (Found != Record.mComponents.end()) {
                if (Component->GetClass()->GetClassName() != Found->mTypeName || Component->GetDefaultComponentName() != Found->mDefaultComponentName) {
                    return false;
                }

                MatchedGuids.insert(Component->GetGuid());
            } else if (Component->GetDefaultComponentName().IsNone()) {
                return false;
            }

            if ((!Component->GetName().IsNone() && (!UObjectSystem::IsValidObjectName(Component->GetName()) || !ComponentNames.insert(Component->GetName()).second)) || (!Component->GetDefaultComponentName().IsNone() && !DefaultNames.insert(Component->GetDefaultComponentName()).second)) {
                return false;
            }
        }

        if (MatchedGuids.size() != Record.mComponents.size()) {
            return false;
        }
    }

    return true;
}

bool FWorldSerializer::LoadInternal(UWorld& World, FArchive& Archive, const UWorld* SourceWorld) {
    if (!Archive.IsLoading() || Archive.HasError() || FAssetRegistry::TryGet() == nullptr) {
        return false;
    }

    if (Archive.SupportsNamedFields() && (!Archive.HasField("FormatVersion") || !Archive.HasField("Actors"))) {
        return false;
    }

    Archive.SetAssetResolver(FAssetRegistry::TryGet());

    Uint32 FormatVersion{};

    Archive.Serialize("FormatVersion", FormatVersion);

    if (Archive.HasError() || (FormatVersion != 4 && (FormatVersion != 3 || !Archive.SupportsNamedFields()))) {
        return false;
    }

    TArray<FActorRecord> Records{};

    if (!SerializeObjectTable(Archive, Records, FormatVersion) || !ValidateObjectTable(World, Records, SourceWorld != nullptr)) {
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
        Archive.BeginObjectScope(std::to_string(Index));

        std::unique_ptr<AActor> Actor{CreateAndLoadActor(Records[Index], Archive)};

        Valid = Actor != nullptr && !Archive.HasError();
        Archive.EndObjectScope();

        if (Valid) {
            LoadedActors.push_back(std::move(Actor));
        }
    }

    Archive.EndArrayScope();

    return Valid && ValidateLoadedActors(LoadedActors, Records) && InstallActors(World, std::move(LoadedActors));
}

bool FWorldSerializer::InstallActors(UWorld& World, TArray<std::unique_ptr<AActor>> Actors) {
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

    for (const std::unique_ptr<AActor>& Actor : World.mPersistentLevel->mActors) {
        Actor->SetWorld(&World, false);
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
            World.InitializeLoadedActor(*World.mPersistentLevel->mActors[Index]);
        }
    }

    if (!World.GetPickingSubsystem().RebuildAccelerationStructure()) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Failed to rebuild the picking acceleration structure after loading the scene.");
    }

    World.MarkStructureDirty();

    return true;
}

TArray<AActor*> FWorldSerializer::DuplicateActors(UWorld& World, const TArray<AActor*>& Actors) {
    if (!CanSerialize(World) || Actors.empty()) {
        return {};
    }

    TArray<FActorRecord> Records{};
    TArray<Uint8> Bytes{};
    FArchiveMemory Writer{Bytes};
    Writer.SetAssetResolver(FAssetRegistry::TryGet());

    for (AActor* Actor : Actors) {
        if (Actor == nullptr || Actor->GetWorld() != &World || Actor->IsBeingDestroyed() || !Actor->HasFinishedSpawning()) {
            return {};
        }

        FActorRecord Record{{Actor->GetGuid(), Actor->GetClass()->GetClassName()}, {}};

        for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
            Record.mComponents.push_back({Component->GetGuid(), Component->GetClass()->GetClassName(), Component->GetDefaultComponentName()});
        }

        Records.push_back(std::move(Record));
        Actor->Save(Writer);
    }

    TMap<FGuid, FGuid> Remapping{};

    if (Writer.HasError() || !RemapObjectTable(World, World, Records, Remapping) || !ValidateObjectTable(World, Records)) {
        return {};
    }

    FArchiveMemory Reader{static_cast<const TArray<Uint8>&>(Bytes)};
    Reader.SetAssetResolver(FAssetRegistry::TryGet());
    const FObjectGuidScope GuidScope{Reader, &Remapping};
    TArray<std::unique_ptr<AActor>> Copies{};

    for (const FActorRecord& Record : Records) {
        std::unique_ptr<AActor> Actor{CreateAndLoadActor(Record, Reader)};

        if (Actor == nullptr) {
            return {};
        }

        const FName UniqueName{UObjectSystem::MakeUniqueObjectName(&World.GetPersistentLevel(), Actor->GetName())};

        if (!Actor->SetName(UniqueName) || !Actor->SetOuter(&World.GetPersistentLevel()) || !UObjectSystem::Register(Actor.get()).IsValid()) {
            return {};
        }

        Actor->SetWorld(&World, false);

        for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
            if (!UObjectSystem::Register(Component.get()).IsValid()) {
                return {};
            }
        }

        Copies.push_back(std::move(Actor));
    }

    if (Reader.GetRemainingBytes() != 0 || !ValidateLoadedActors(Copies, Records)) {
        return {};
    }

    for (const std::unique_ptr<AActor>& Actor : Copies) {
        if (!Actor->ResolveLoadedReferences()) {
            return {};
        }
    }

    TArray<TObjectRef<AActor>> Installed{};

    for (std::unique_ptr<AActor>& Actor : Copies) {
        Installed.push_back(TObjectRef<AActor>{Actor.get()});
        World.mPersistentLevel->mActors.push_back(std::move(Actor));
    }

    {
        const UWorld::FActorDispatchScope Dispatch{&World};

        for (const TObjectRef<AActor>& Entry : Installed) {
            if (AActor* Actor{Entry.Get()}) {
                World.InitializeLoadedActor(*Actor);
            }
        }
    }

    TArray<AActor*> Result{};

    for (const TObjectRef<AActor>& Entry : Installed) {
        AActor* Actor{Entry.Get()};

        if (Actor == nullptr || Actor->IsBeingDestroyed()) {
            for (const TObjectRef<AActor>& Copy : Installed) {
                if (AActor* Remaining{Copy.Get()}) {
                    World.DestroyActor(Remaining);
                }
            }

            World.FlushPendingDestroyActors();
            return {};
        }

        Result.push_back(Actor);
    }

    return Result;
}
