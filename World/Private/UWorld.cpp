#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "World/UWorld.h"
#include "Core/Stat/Stat.h"

#include <algorithm>
#include <cmath>
#include <random>
#include "World/AActor.h"
#include "World/Component/UCameraComponent.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Subsystem/UCameraSubsystem.h"
#include "World/Subsystem/UCollisionSubsystem.h"
#include "World/Subsystem/UPickingSubsystem.h"
#include "World/Subsystem/URenderSubsystem.h"
#include "World/Subsystem/UTextSubsystem.h"
#include "World/Subsystem/UBillboardSubsystem.h"
#include "World/Subsystem/ULightSubsystem.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/ULightComponent.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Asset/UMesh.h"
#include "CoreUObject/UObjectSystem.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Core/Console/Console.h"

#include <ranges>

UWorld::FActorDispatchScope::FActorDispatchScope(UWorld* World)
	: mWorld{World} {
    if (mWorld != nullptr) {
        ++mWorld->mActorDispatchDepth;
    }
}

UWorld::FActorDispatchScope::~FActorDispatchScope() {
    if (mWorld != nullptr) {
        --mWorld->mActorDispatchDepth;
    }
}

UWorld::UWorld() = default;

UWorld::~UWorld() {
    CleanupWorld();
}

void UWorld::Initialize(EWorldType WorldType) {
    if (mInitialized || mCleaningUp) {
        return;
    }

    if (!UObjectSystem::Register(this).IsValid()) {
        ErrorHandler::Report("UWorld", "Cannot register world", ErrorHandler::EErrorLevel::Critical);
    }

    mWorldType = WorldType;
    mTime.Reset();
    mPersistentLevel = std::make_unique<ULevel>(*this);
    ErrorHandler::Report(!UObjectSystem::Register(mPersistentLevel.get()).IsValid(), "UWorld", "Cannot register persistent level", ErrorHandler::EErrorLevel::Critical);
    mInitialized = true;
    InitializeSubsystems();
}

void UWorld::CleanupWorld() {
    if (!mInitialized || mCleaningUp) {
        return;
    }

    if (mBTickingActors || mBeginningPlay || mEndingPlay || mActorDispatchDepth > 0 || mFlushingActors) {
        ErrorHandler::Report("UWorld", "Cannot clean up a world during actor callbacks", ErrorHandler::EErrorLevel::Critical);
    }

    mCleaningUp = true;
    EndPlay(EEndPlayReason::Quit);
    NotifyWorldChanged(EWorldChange::Destroying);
    mObservers.clear();
    ClearActors();
    DeinitializeSubsystems();
    UObjectSystem::Unregister(mPersistentLevel.get(), mPersistentLevel->GetHandle());
    mPersistentLevel.reset();
    mPendingDestroyActors.clear();
    mTickActors.clear();
    mTickComponents.clear();
    mTickComponentsNeedCompaction = false;
    mTickActorsNeedCompaction = false;
    mEndPlayRequested = false;
    mAssetRegistry = nullptr;
    mAssetRegistryMutator = nullptr;
    mInitialized = false;
    mCleaningUp = false;
}

bool UWorld::IsInitialized() const {
    return mInitialized;
}

EWorldType UWorld::GetWorldType() const {
    return mWorldType;
}

ULevel& UWorld::GetPersistentLevel() {
    if (mPersistentLevel == nullptr) {
        ErrorHandler::Report("UWorld", "World is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    return *mPersistentLevel;
}

const ULevel& UWorld::GetPersistentLevel() const {
    if (mPersistentLevel == nullptr) {
        ErrorHandler::Report("UWorld", "World is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    return *mPersistentLevel;
}

void UWorld::BeginPlay() {
    if (!mInitialized || mHasBegunPlay || mBeginningPlay || mWorldType != EWorldType::Game || mCleaningUp || mLoadingScene || mEndingPlay) {
        return;
    }

    mTime.Reset();
    mBeginningPlay = true;

    for (std::size_t Index{}; Index < mPersistentLevel->mActors.size() && !mEndPlayRequested; ++Index) {
        AActor* Actor{mPersistentLevel->mActors[Index].get()};

        if (!Actor->IsBeingDestroyed()) {
            Actor->InitializeComponents();
        }
    }

    mHasBegunPlay = true;

    const std::size_t Count{mPersistentLevel->mActors.size()};

    for (std::size_t Index{}; Index < Count && !mEndPlayRequested; ++Index) {
        AActor* Actor{mPersistentLevel->mActors[Index].get()};

        if (!Actor->IsBeingDestroyed()) {
            Actor->DispatchBeginPlay();
        }
    }

    mBeginningPlay = false;

    if (mEndPlayRequested) {
        EndPlay(mEndPlayReason);
    }
}

void UWorld::EndPlay(EEndPlayReason Reason) {
    if (mEndingPlay || (!mHasBegunPlay && !mBeginningPlay)) {
        return;
    }

    if (mBeginningPlay || mBTickingActors || mActorDispatchDepth > 0) {
        mEndPlayRequested = true;
        mEndPlayReason = Reason;
        return;
    }

    mEndPlayRequested = false;
    mEndingPlay = true;
    mHasBegunPlay = false;

    for (std::size_t Index{mPersistentLevel->mActors.size()}; Index > 0; --Index) {
        mPersistentLevel->mActors[Index - 1]->DispatchEndPlay(Reason);
    }

    mEndingPlay = false;
}

bool UWorld::HasBegunPlay() const {
    return mHasBegunPlay;
}

TSubsystemCollection<UWorldSubsystem, UWorld>& UWorld::GetSubsystems() {
    return mSubsystems;
}

void UWorld::RefreshActorTicks() {
    for (const std::unique_ptr<AActor>& Actor : mPersistentLevel->mActors) {
        for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
            Component->UpdateTickRegistration();
        }

        Actor->UpdateTickRegistration();
    }
}

AActor* UWorld::SpawnStaticMeshActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const FVector3& Position) {
    if (mAssetRegistry == nullptr || mAssetRegistry->ResolveAsset<UMesh>(MeshHandle) == nullptr) {
        return nullptr;
    }

    AActor* Actor{SpawnActorDeferred<AActor>()};

    if (Actor == nullptr) {
        return nullptr;
    }

    UStaticMeshComponent* MeshComponent{Actor->AddComponent<UStaticMeshComponent>()};

    if (MeshComponent == nullptr) {
        DestroyActor(Actor);
        return nullptr;
    }

    Actor->SetRootComponent(MeshComponent);

    MeshComponent->SetMeshHandle(MeshHandle);
    MeshComponent->SetPipelineHandle(PipelineHandle);
    MeshComponent->SetMaterialHandle(MaterialHandle);

    MeshComponent->SetRelativeLocation(FVector3{Position.mX, Position.mY, Position.mZ});

    return FinishSpawningActor(Actor, Actor->GetActorTransform()) ? Actor : nullptr;
}

bool UWorld::DestroyActor(AActor* Actor) {
    return DestroyActorInternal(Actor, EEndPlayReason::Destroyed);
}

bool UWorld::DestroyActorInternal(AActor* Actor, EEndPlayReason Reason) {
    if (!mInitialized || Actor == nullptr || Actor->GetWorld() != this) {
        return false;
    }

    if (Actor->mBIsBeingDestroyed) {
        return true;
    }

    const FActorDispatchScope Dispatch{this};

    Actor->mBIsBeingDestroyed = true;
    mPendingDestroyActors.push_back(Actor);
    Actor->UpdateTickRegistration();
    NotifyWorldChanged(EWorldChange::ActorRemoving, Actor);
    Actor->DispatchEndPlay(Reason);

    if (Reason == EEndPlayReason::Destroyed) {
        Actor->Destroyed();
    }

    const TArray<UActorComponent*> Components{Actor->GetComponentSnapshot()};

    for (UActorComponent* Component : Components) {
        if (!Component->IsBeingDestroyed() && Component->GetTypeInfo()->IsA(USceneComponent::StaticTypeInfo())) {
            USceneComponent* SceneComponent{static_cast<USceneComponent*>(Component)};
            const std::vector<TObjectRef<USceneComponent>> Children{SceneComponent->GetChildren()};

            for (const TObjectRef<USceneComponent>& ChildReference : Children) {
                USceneComponent* Child{ChildReference.Get()};

                if (Child != nullptr && Child->GetOwner() != Actor) {
                    Child->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
                }
            }

            if (SceneComponent->GetParent() != nullptr && SceneComponent->GetParent()->GetOwner() != Actor) {
                SceneComponent->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
            }
        }

        Component->UnregisterComponent();
    }

    for (UActorComponent* Component : Components) {
        UObjectSystem::Unregister(Component, Component->GetHandle());
    }

    UObjectSystem::Unregister(Actor, Actor->GetHandle());
    MarkStructureDirty();

    return true;
}

void UWorld::FlushPendingDestroyActors() {
    if (!mInitialized || mBTickingActors || mBeginningPlay || mEndingPlay || mFlushingActors || mActorDispatchDepth > 0) {
        return;
    }

    mFlushingActors = true;

    bool RemovedAnyActor{};

    while (!mPendingDestroyActors.empty()) {
        AActor* Actor{mPendingDestroyActors.back()};

        mPendingDestroyActors.pop_back();

        const auto Iterator{std::find_if(mPersistentLevel->mActors.rbegin(), mPersistentLevel->mActors.rend(), [Actor](const std::unique_ptr<AActor>& Candidate) {
            return Candidate.get() == Actor;
        })};

        if (Iterator == mPersistentLevel->mActors.rend()) {
            continue;
        }

        std::unique_ptr<AActor> RemovedActor{std::move(*Iterator)};

        mPersistentLevel->mActors.erase(std::prev(Iterator.base()));
        Actor->SetWorld(nullptr);
        UObjectSystem::Unregister(Actor, Actor->GetHandle());
        RemovedAnyActor = true;
    }

    const std::size_t ActorCount{mPersistentLevel->mActors.size()};

    for (std::size_t Index{}; Index < ActorCount; ++Index) {
        std::vector<std::unique_ptr<UActorComponent>> RetiredComponents{std::move(mPersistentLevel->mActors[Index]->mPendingDestroyComponents)};
    }

    mFlushingActors = false;

    if (RemovedAnyActor) {
        MarkStructureDirty();
    }
}

void UWorld::AttachActor(AActor* Child, AActor* Parent) {
    if (Child == nullptr || Parent == nullptr || Child == Parent) {
        return;
    }

    USceneComponent* ChildRoot{Child->GetRootComponent()};
    USceneComponent* ParentRoot{Parent->GetRootComponent()};

    if (ChildRoot == nullptr || ParentRoot == nullptr) {
        return;
    }

    // 이미 같은 부모라면 변경 없음
    if (ChildRoot->GetParent() == ParentRoot) {
        return;
    }

    ChildRoot->AttachToComponent(ParentRoot);
    MarkStructureDirty();
}

void UWorld::DetachActor(AActor* Actor) {
    if (Actor == nullptr) {
        return;
    }

    USceneComponent* RootComponent{Actor->GetRootComponent()};

    if (RootComponent == nullptr) {
        return;
    }

    if (RootComponent->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform)) {
        MarkStructureDirty();
    }
}

bool UWorld::RenameActor(AActor* Actor, const FName& NewName) {
    if (Actor == nullptr) {
        return false;
    }

    if (Actor->GetWorld() != this) {
        return false;
    }

    if (Actor->GetName() == NewName) {
        return false;
    }

    return Actor->Rename(NewName);
}

const TArray<std::unique_ptr<AActor>>& UWorld::GetActors() const {
    return GetPersistentLevel().GetActors();
}

void UWorld::InitializeSubsystems() {
    mSubsystems.Initialize(*this);
    mSubsystems.Add<URenderSubsystem>();
    mSubsystems.Add<UCollisionSubsystem>();
    mSubsystems.Add<UPickingSubsystem>();
    mSubsystems.Add<UCameraSubsystem>();
    mSubsystems.Add<UTextSubsystem>();
    mSubsystems.Add<UOverlaySubsystem>();
    mSubsystems.Add<UBillboardSubsystem>();
    mSubsystems.Add<ULightSubsystem>();
}

void UWorld::DeinitializeSubsystems() {
    mSubsystems.Deinitialize();
}

UTextSubsystem& UWorld::GetTextSubsystem() {
    return *mSubsystems.Get<UTextSubsystem>();
}

const UTextSubsystem& UWorld::GetTextSubsystem() const {
    return *mSubsystems.Get<UTextSubsystem>();
}

UOverlaySubsystem& UWorld::GetOverlaySubsystem() {
    return *mSubsystems.Get<UOverlaySubsystem>();
}

const UOverlaySubsystem& UWorld::GetOverlaySubsystem() const {
    return *mSubsystems.Get<UOverlaySubsystem>();
}

ULightSubsystem& UWorld::GetLightSubsystem() {
    return *mSubsystems.Get<ULightSubsystem>();
}

const ULightSubsystem& UWorld::GetLightSubsystem() const {
    return *mSubsystems.Get<ULightSubsystem>();
}

void UWorld::BuildSceneRenderData(FSceneRenderData& Scene) {
    mSubsystems.Get<URenderSubsystem>()->BuildRenderProbes(Scene);

    mSubsystems.Get<ULightSubsystem>()->BuildLightProbes(Scene);
    mSubsystems.Get<UTextSubsystem>()->BuildTextProbes(Scene);
    mSubsystems.Get<UBillboardSubsystem>()->BuildRenderProbes(Scene);
}

void UWorld::BuildOverlayRenderData(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) {
    mSubsystems.Get<UOverlaySubsystem>()->BuildRenderProbes(Overlay, SelectedActor);
}

void UWorld::MarkStructureDirty() {
    ++mStructureRevision;
    NotifyWorldChanged(EWorldChange::StructureChanged);
}

Uint64 UWorld::GetStructureRevision() const {
    return mStructureRevision;
}

void UWorld::Tick(float DeltaTime) {
    if (!mInitialized || mBTickingActors || mCleaningUp || mLoadingScene || mBeginningPlay || mEndingPlay || mActorDispatchDepth > 0) {
        return;
    }

    if (mEndPlayRequested) {
        EndPlay(mEndPlayReason);
    }

    if (mWorldType == EWorldType::Game && !mHasBegunPlay) {
        FlushPendingDestroyActors();
        return;
    }

    mTime.Tick(static_cast<double>(DeltaTime));

    const float WorldDeltaTime{static_cast<float>(mTime.GetDeltaSeconds())};

    if (WorldDeltaTime > 0.0f) {
        const Stat::FScopedWorldTickStatTimer TickStat{0};
        Stat::FWorldTickStats* TickStats{Stat::GetActiveWorldTickStats()};
        const std::size_t ActorCount{mTickActors.size()};
        const std::size_t ComponentCount{mTickComponents.size()};

        mBTickingActors = true;

        for (std::size_t Index{}; Index < ActorCount && !mEndPlayRequested; ++Index) {
            AActor* Actor{mTickActors[Index]};

            if (Actor == nullptr || Actor->IsBeingDestroyed()) {
                continue;
            }

            if (TickStats != nullptr) {
                ++TickStats->mActorTickCount;
            }

            Actor->Tick(WorldDeltaTime);
        }

        for (std::size_t Index{}; Index < ComponentCount && !mEndPlayRequested; ++Index) {
            UActorComponent* Component{mTickComponents[Index]};

            if (Component == nullptr || Component->IsBeingDestroyed() || Component->GetOwner()->IsBeingDestroyed()) {
                continue;
            }

            if (TickStats != nullptr) {
                ++TickStats->mComponentVisitCount;
                ++TickStats->mComponentTickCount;
            }

            Component->TickComponent(WorldDeltaTime);
        }

        FinishTicks();
    }

    if (mEndPlayRequested) {
        EndPlay(mEndPlayReason);
    }

    FlushPendingDestroyActors();
}

void UWorld::RegisterTickActor(AActor* Actor) {
    if (Actor->mTickIndex != std::numeric_limits<std::size_t>::max()) {
        return;
    }

    mTickActors.push_back(Actor);
    Actor->mTickIndex = mTickActors.size() - 1;
}

void UWorld::UnregisterTickActor(AActor* Actor) {
    const std::size_t Index{Actor->mTickIndex};

    if (Index == std::numeric_limits<std::size_t>::max()) {
        return;
    }

    Actor->mTickIndex = std::numeric_limits<std::size_t>::max();

    if (mBTickingActors) {
        mTickActors[Index] = nullptr;
        mTickActorsNeedCompaction = true;
    } else {
        if (Index + 1 < mTickActors.size()) {
            AActor* LastActor{mTickActors.back()};

            mTickActors[Index] = LastActor;
            LastActor->mTickIndex = Index;
        }

        mTickActors.pop_back();
    }
}

void UWorld::RegisterTickComponent(UActorComponent* Component) {
    if (Component->mTickIndex != std::numeric_limits<std::size_t>::max()) {
        return;
    }

    mTickComponents.push_back(Component);
    Component->mTickIndex = mTickComponents.size() - 1;
}

void UWorld::UnregisterTickComponent(UActorComponent* Component) {
    const std::size_t Index{Component->mTickIndex};

    if (Index == std::numeric_limits<std::size_t>::max()) {
        return;
    }

    Component->mTickIndex = std::numeric_limits<std::size_t>::max();

    if (mBTickingActors) {
        mTickComponents[Index] = nullptr;
        mTickComponentsNeedCompaction = true;
    } else {
        if (Index + 1 < mTickComponents.size()) {
            UActorComponent* LastComponent{mTickComponents.back()};

            mTickComponents[Index] = LastComponent;
            LastComponent->mTickIndex = Index;
        }

        mTickComponents.pop_back();
    }
}

void UWorld::FinishTicks() {
    mBTickingActors = false;

    if (mTickActorsNeedCompaction) {
        std::erase(mTickActors, nullptr);

        for (std::size_t Index{}; Index < mTickActors.size(); ++Index) {
            mTickActors[Index]->mTickIndex = Index;
        }

        mTickActorsNeedCompaction = false;
    }

    if (mTickComponentsNeedCompaction) {
        std::erase(mTickComponents, nullptr);

        for (std::size_t Index{}; Index < mTickComponents.size(); ++Index) {
            mTickComponents[Index]->mTickIndex = Index;
        }

        mTickComponentsNeedCompaction = false;
    }
}

FWorldTime& UWorld::GetTime() {
    return mTime;
}

const FWorldTime& UWorld::GetTime() const {
    return mTime;
}

URenderSubsystem& UWorld::GetRenderSubsystem() {
    return *mSubsystems.Get<URenderSubsystem>();
}

const URenderSubsystem& UWorld::GetRenderSubsystem() const {
    return *mSubsystems.Get<URenderSubsystem>();
}

UCollisionSubsystem& UWorld::GetCollisionSubsystem() {
    return *mSubsystems.Get<UCollisionSubsystem>();
}

const UCollisionSubsystem& UWorld::GetCollisionSubsystem() const {
    return *mSubsystems.Get<UCollisionSubsystem>();
}

UPickingSubsystem& UWorld::GetPickingSubsystem() {
    return *mSubsystems.Get<UPickingSubsystem>();
}

const UPickingSubsystem& UWorld::GetPickingSubsystem() const {
    return *mSubsystems.Get<UPickingSubsystem>();
}

UCameraSubsystem& UWorld::GetCameraSubsystem() {
    return *mSubsystems.Get<UCameraSubsystem>();
}

const UCameraSubsystem& UWorld::GetCameraSubsystem() const {
    return *mSubsystems.Get<UCameraSubsystem>();
}

UBillboardSubsystem& UWorld::GetBillboardSubsystem() {
    return *mSubsystems.Get<UBillboardSubsystem>();
}

const UBillboardSubsystem& UWorld::GetBillboardSubsystem() const {
    return *mSubsystems.Get<UBillboardSubsystem>();
}

AActor* UWorld::AddActorInternal(std::unique_ptr<AActor> InActor) {
    if (!mInitialized || mCleaningUp || mEndingPlay || !InActor || InActor->GetWorld() != nullptr || InActor->IsBeingDestroyed()) {
        return nullptr;
    }

    const FActorDispatchScope Dispatch{this};
    AActor* Actor{InActor.get()};

    if (!Actor->SetOuter(mPersistentLevel.get()) || !UObjectSystem::Register(Actor).IsValid()) {
        return nullptr;
    }

    mPersistentLevel->mActors.push_back(std::move(InActor));
    Actor->SetWorld(this);

    return Actor->IsBeingDestroyed() ? nullptr : Actor;
}

AActor* UWorld::AddActor(std::unique_ptr<AActor> InActor) {
    const FActorDispatchScope Dispatch{this};
    const FTransform Transform{InActor != nullptr ? InActor->GetActorTransform() : FTransform{}};
    AActor* Actor{AddActorInternal(std::move(InActor))};

    if (Actor == nullptr) {
        return nullptr;
    }

    Actor->PostActorCreated();

    return FinishSpawningActor(Actor, Transform) ? Actor : nullptr;
}

AActor* UWorld::SpawnActor(const FTypeInfo& Type, const FTransform& Transform) {
    const FActorDispatchScope Dispatch{this};
    AActor* Actor{SpawnActorDeferred(Type, Transform)};

    return Actor != nullptr && FinishSpawningActor(Actor, Transform) ? Actor : nullptr;
}

AActor* UWorld::SpawnActorDeferred(const FTypeInfo& Type, const FTransform& Transform) {
    if (!mInitialized || mCleaningUp || mEndingPlay || Type.mCreator == nullptr || !Type.IsA(AActor::StaticTypeInfo())) {
        return nullptr;
    }

    const FActorDispatchScope Dispatch{this};
    std::unique_ptr<UObject> Object{Type.mCreator()};

    if (Object == nullptr || !Object->GetTypeInfo()->IsA(AActor::StaticTypeInfo())) {
        return nullptr;
    }

    std::unique_ptr<AActor> Owner{static_cast<AActor*>(Object.release())};
    AActor* Actor{AddActorInternal(std::move(Owner))};

    if (Actor == nullptr) {
        return nullptr;
    }

    Actor->SetActorTransform(Transform);
    Actor->PostActorCreated();

    return Actor->IsBeingDestroyed() ? nullptr : Actor;
}

bool UWorld::FinishSpawningActor(AActor* Actor, const FTransform& Transform) {
    if (Actor == nullptr || Actor->GetWorld() != this || Actor->mHasFinishedSpawning || Actor->mFinishingSpawning || Actor->IsBeingDestroyed()) {
        return false;
    }

    const FActorDispatchScope Dispatch{this};
    const bool HadRootComponent{Actor->GetRootComponent() != nullptr};

    Actor->mFinishingSpawning = true;
    Actor->SetActorTransform(Transform);
    Actor->OnConstruction(Transform);

    if (!HadRootComponent) {
        Actor->SetActorTransform(Transform);
    }

    Actor->mFinishingSpawning = false;

    if (Actor->IsBeingDestroyed()) {
        return false;
    }

    Actor->mHasFinishedSpawning = true;
    Actor->RegisterAllComponents();

    if (mHasBegunPlay && !mLoadingScene) {
        Actor->DispatchBeginPlay();
    }

    if (Actor->IsBeingDestroyed()) {
        return false;
    }

    NotifyWorldChanged(EWorldChange::ActorAdded, Actor);
    MarkStructureDirty();

    return !Actor->IsBeingDestroyed();
}

void UWorld::InitializeLoadedActor(AActor& Actor) {
    const FActorDispatchScope Dispatch{this};

    Actor.SetWorld(this);

    if (Actor.IsBeingDestroyed()) {
        return;
    }

    Actor.PostLoad();

    if (Actor.IsBeingDestroyed()) {
        return;
    }

    Actor.mHasFinishedSpawning = true;
    Actor.RegisterAllComponents();

    if (mHasBegunPlay && !mLoadingScene) {
        Actor.DispatchBeginPlay();
    }

    if (!Actor.IsBeingDestroyed()) {
        NotifyWorldChanged(EWorldChange::ActorAdded, &Actor);
        MarkStructureDirty();
    }
}

const IAssetRegistry* UWorld::GetAssetRegistry() const {
    return mAssetRegistry;
}

void UWorld::ClearActors() {
    mPendingDestroyActors.reserve(mPersistentLevel->mActors.size());

    const std::size_t Count{mPersistentLevel->mActors.size()};

    for (std::size_t Index{}; Index < Count; ++Index) {
        AActor* Actor{mPersistentLevel->mActors[Index].get()};

        if (Actor->GetWorld() == this) {
            DestroyActorInternal(Actor, mCleaningUp ? EEndPlayReason::Quit : EEndPlayReason::LevelTransition);
        } else if (!Actor->mBIsBeingDestroyed) {
            Actor->mBIsBeingDestroyed = true;
            mPendingDestroyActors.push_back(Actor);
        }
    }

    FlushPendingDestroyActors();
}

FName UWorld::MakeUniqueObjectName(FName SourceName) {
    return mInitialized ? UObjectSystem::MakeUniqueObjectName(mPersistentLevel.get(), SourceName) : FName{};
}

AActor* UWorld::FindActorByName(FName InName) const {
    UObject* Object{mInitialized ? UObjectSystem::FindObject(mPersistentLevel.get(), InName) : nullptr};

    AActor* Actor{Object != nullptr && Object->GetTypeInfo()->IsA(AActor::StaticTypeInfo()) ? static_cast<AActor*>(Object) : nullptr};

    return Actor != nullptr && !Actor->IsBeingDestroyed() ? Actor : nullptr;
}

void UWorld::SetAssetRegistry(const IAssetRegistry* InAssetRegistry, IAssetRegistryMutator* InAssetRegistryMutator) {
    mAssetRegistry = InAssetRegistry;
    mAssetRegistryMutator = InAssetRegistryMutator;
}

IAssetRegistryMutator* UWorld::GetAssetRegistryMutator() const {
    return mAssetRegistryMutator;
}

void UWorld::AddObserver(IWorldObserver& Observer) {
    if (std::ranges::find(mObservers, &Observer) == mObservers.end()) {
        mObservers.push_back(&Observer);
    }
}

void UWorld::RemoveObserver(IWorldObserver& Observer) {
    std::erase(mObservers, &Observer);
}

void UWorld::NotifyWorldChanged(EWorldChange Change, AActor* Actor) {
    const FActorDispatchScope Dispatch{this};
    const std::vector<IWorldObserver*> Observers{mObservers};

    for (IWorldObserver* Observer : Observers) {
        if (std::ranges::find(mObservers, Observer) != mObservers.end()) {
            Observer->OnWorldChanged(*this, Change, Actor);
        }
    }
}
