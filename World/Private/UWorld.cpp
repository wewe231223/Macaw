#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Base/TStateFlags.h"
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

UWorld::FLifecycleScope::FLifecycleScope(UWorld& World)
	: mWorld{World} {
    ++mWorld.mLifecycleDepth;
}

UWorld::FLifecycleScope::~FLifecycleScope() {
    --mWorld.mLifecycleDepth;

    if (mWorld.mLifecycleDepth == 0) {
        mWorld.ProcessPendingLifecycleChanges();
    }
}

bool UWorld::IsDeferringLifecycleChanges() const {
    return mLifecycleDepth > 0;
}

void UWorld::ProcessPendingLifecycleChanges() {
    if (IsDeferringLifecycleChanges() || mState.IsSet(EStateFlag::ProcessingLifecycleRequests) || mPendingLifecycleRequests.empty()) {
        return;
    }

    const TScopedStateFlag ProcessingRequests{mState, EStateFlag::ProcessingLifecycleRequests};

    while (!mPendingLifecycleRequests.empty()) {
        TArray<FLifecycleRequest> Requests{std::move(mPendingLifecycleRequests)};

        mPendingLifecycleRequests.clear();

        for (FLifecycleRequest& Request : Requests) {
            switch (Request.mType) {
            case ELifecycleRequestType::AddActor: {
                std::unique_ptr<AActor> Owner{static_cast<AActor*>(Request.mOwner.release())};
                AActor* Actor{Owner.get()};

                mPersistentLevel->mActors.push_back(std::move(Owner));

                if (!Actor->IsBeingDestroyed()) {
                    Actor->OnAddedToWorld();

                    if (!Actor->IsBeingDestroyed()) {
                        Actor->PostActorCreated();
                    }
                }
                break;
            }
            case ELifecycleRequestType::FinishSpawningActor: {
                AActor* Actor{static_cast<AActor*>(Request.mTarget)};

                Actor->mState.Clear(AActor::EStateFlag::FinishingSpawning);
                FinishSpawningActor(Actor, Request.mTransform);
                break;
            }
            case ELifecycleRequestType::AddComponent: {
                std::unique_ptr<UActorComponent> Owner{static_cast<UActorComponent*>(Request.mOwner.release())};
                UActorComponent* Component{Owner.get()};
                AActor* Actor{Component->GetOwner()};

                Actor->mComponents.push_back(std::move(Owner));

                if (Actor->mState.IsSet(AActor::EStateFlag::BeingDestroyed)) {
                    Component->DestroyComponent();
                } else {
                    Actor->FinishAddingComponent(*Component);
                }

                if (Actor->HasFinishedSpawning() && !Actor->IsBeingDestroyed()) {
                    MarkStructureDirty();
                }
                break;
            }
            case ELifecycleRequestType::RegisterComponent:
                static_cast<UActorComponent*>(Request.mTarget)->RegisterComponent(this);
                break;
            case ELifecycleRequestType::UnregisterComponent:
                static_cast<UActorComponent*>(Request.mTarget)->UnregisterComponent();
                break;
            case ELifecycleRequestType::AdvanceComponent: {
                UActorComponent* Component{static_cast<UActorComponent*>(Request.mTarget)};

                Component->GetOwner()->OnComponentRegistered(*Component);
                break;
            }
            case ELifecycleRequestType::DestroyActor: {
                AActor* Actor{static_cast<AActor*>(Request.mTarget)};

                Actor->mState.Clear(AActor::EStateFlag::DestroyRequested);
                DestroyActorInternal(Actor, Request.mReason);
                break;
            }
            case ELifecycleRequestType::DestroyComponent: {
                UActorComponent* Component{static_cast<UActorComponent*>(Request.mTarget)};

                Component->mState.Clear(UActorComponent::EStateFlag::DestroyRequested);
                Component->DestroyComponent(Request.mPromoteChildren);
                break;
            }
            }
        }
    }
}

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

UWorld::UWorld() {
    InitializeClass(StaticClass());
}

UWorld::~UWorld() {
    CleanupWorld();
}

void UWorld::Initialize(EWorldType WorldType) {
    if (mState.IsSet(EStateFlag::Initialized) || mState.IsSet(EStateFlag::CleaningUp)) {
        return;
    }

    if (!UObjectSystem::Register(this).IsValid()) {
        ErrorHandler::Report("UWorld", "Cannot register world", ErrorHandler::EErrorLevel::Critical);
    }

    mWorldType = WorldType;
    mTime.Reset();
    mPersistentLevel = NewObject<ULevel>(*this);
    ErrorHandler::Report(!UObjectSystem::Register(mPersistentLevel.get()).IsValid(), "UWorld", "Cannot register persistent level", ErrorHandler::EErrorLevel::Critical);
    mState.Set(EStateFlag::Initialized);
    InitializeSubsystems();
}

void UWorld::CleanupWorld() {
    mInput.Reset();
    if (mState.IsSet(EStateFlag::CleaningUp)) {
        return;
    }

    if (!mState.IsSet(EStateFlag::Initialized)) {
        BindScene({});
        return;
    }

    if (mState.IsSet(EStateFlag::Ticking) || mState.IsSet(EStateFlag::BeginningPlay) || mState.IsSet(EStateFlag::EndingPlay) || mActorDispatchDepth > 0 || mState.IsSet(EStateFlag::FlushingActors) || IsDeferringLifecycleChanges() || mState.IsSet(EStateFlag::ProcessingLifecycleRequests)) {
        ErrorHandler::Report("UWorld", "Cannot clean up a world during actor callbacks", ErrorHandler::EErrorLevel::Critical);
    }

    mState.Set(EStateFlag::CleaningUp);
    EndPlay(EEndPlayReason::Quit);
    NotifyWorldChanged(EWorldChange::Destroying);
    mObservers.clear();
    ClearActors();
    SendSceneUpdates();
    BindScene({});
    DeinitializeSubsystems();
    UObjectSystem::Unregister(mPersistentLevel.get(), mPersistentLevel->GetHandle());
    mPersistentLevel.reset();
    mPendingDestroyActors.clear();
    mPendingLifecycleRequests.clear();
    mActorTickFunctions.clear();
    mComponentTickFunctions.clear();
    mState.Clear(EStateFlag::ComponentTickFunctionsNeedCompaction);
    mState.Clear(EStateFlag::ActorTickFunctionsNeedCompaction);
    mState.Clear(EStateFlag::EndPlayRequested);
    mState.Clear(EStateFlag::Initialized);
    mState.Clear(EStateFlag::CleaningUp);
}

bool UWorld::IsInitialized() const {
    return mState.IsSet(EStateFlag::Initialized);
}

EWorldType UWorld::GetWorldType() const {
    return mWorldType;
}

bool UWorld::IsGameWorld() const {
    return mWorldType == EWorldType::Game || mWorldType == EWorldType::PIE;
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
    if (!mState.IsSet(EStateFlag::Initialized) || mState.IsSet(EStateFlag::HasBegunPlay) || mState.IsSet(EStateFlag::BeginningPlay) || !IsGameWorld() || mState.IsSet(EStateFlag::CleaningUp) || mState.IsSet(EStateFlag::LoadingScene) || mState.IsSet(EStateFlag::EndingPlay)) {
        return;
    }

    mTime.Reset();
    {
        const TScopedStateFlag BeginningPlay{mState, EStateFlag::BeginningPlay};

        {
            const FLifecycleScope Lifecycle{*this};

            for (const std::unique_ptr<AActor>& Actor : mPersistentLevel->mActors) {
                if (mState.IsSet(EStateFlag::EndPlayRequested)) {
                    break;
                }

                if (!Actor->IsBeingDestroyed()) {
                    Actor->InitializeComponents();
                }
            }
        }

        mState.Set(EStateFlag::HasBegunPlay);

        {
            const FLifecycleScope Lifecycle{*this};

            for (const std::unique_ptr<AActor>& Actor : mPersistentLevel->mActors) {
                if (mState.IsSet(EStateFlag::EndPlayRequested)) {
                    break;
                }

                if (!Actor->IsBeingDestroyed()) {
                    Actor->DispatchBeginPlay();
                }
            }
        }
    }

    if (mState.IsSet(EStateFlag::EndPlayRequested)) {
        EndPlay(mEndPlayReason);
    }
}

void UWorld::EndPlay(EEndPlayReason Reason) {
    mInput.Reset();
    if (mState.IsSet(EStateFlag::EndingPlay) || (!mState.IsSet(EStateFlag::HasBegunPlay) && !mState.IsSet(EStateFlag::BeginningPlay))) {
        return;
    }

    if (mState.IsSet(EStateFlag::BeginningPlay) || mState.IsSet(EStateFlag::Ticking) || mActorDispatchDepth > 0 || IsDeferringLifecycleChanges() || mState.IsSet(EStateFlag::ProcessingLifecycleRequests)) {
        mState.Set(EStateFlag::EndPlayRequested);
        mEndPlayReason = Reason;
        return;
    }

    mState.Clear(EStateFlag::EndPlayRequested);
    mState.Clear(EStateFlag::HasBegunPlay);

    const TScopedStateFlag EndingPlay{mState, EStateFlag::EndingPlay};

    for (std::size_t Index{mPersistentLevel->mActors.size()}; Index > 0; --Index) {
        mPersistentLevel->mActors[Index - 1]->DispatchEndPlay(Reason);
    }
}

bool UWorld::HasBegunPlay() const {
    return mState.IsSet(EStateFlag::HasBegunPlay);
}

FWorldInput& UWorld::GetInput() {
    return mInput;
}

const FWorldInput& UWorld::GetInput() const {
    return mInput;
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

AActor* UWorld::SpawnStaticMeshActor(const FAssetHandle& MeshHandle, const FVector3& Position) {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    if (Registry == nullptr || Registry->ResolveAsset<UMesh>(MeshHandle) == nullptr) {
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

    MeshComponent->SetRelativeLocation(FVector3{Position.mX, Position.mY, Position.mZ});

    return FinishSpawningActor(Actor, Actor->GetActorTransform()) ? Actor : nullptr;
}

bool UWorld::DestroyActor(AActor* Actor) {
    return DestroyActorInternal(Actor, EEndPlayReason::Destroyed);
}

bool UWorld::DestroyActorInternal(AActor* Actor, EEndPlayReason Reason) {
    if (!mState.IsSet(EStateFlag::Initialized) || Actor == nullptr || Actor->GetWorld() != this) {
        return false;
    }

    if (Actor->mState.IsSet(AActor::EStateFlag::BeingDestroyed)) {
        return true;
    }

    if (IsDeferringLifecycleChanges()) {
        if (!Actor->mState.IsSet(AActor::EStateFlag::DestroyRequested)) {
            Actor->mState.Set(AActor::EStateFlag::DestroyRequested);
            mPendingLifecycleRequests.push_back({ELifecycleRequestType::DestroyActor, Actor, {}, {}, Reason});
        }
        return true;
    }

    const FActorDispatchScope Dispatch{this};

    Actor->mState.Set(AActor::EStateFlag::BeingDestroyed);
    mPendingDestroyActors.push_back(Actor);
    Actor->UpdateTickRegistration();
    NotifyWorldChanged(EWorldChange::ActorRemoving, Actor);
    Actor->DispatchEndPlay(Reason);

    if (Reason == EEndPlayReason::Destroyed) {
        Actor->Destroyed();
    }

    const TArray<UActorComponent*> Components{Actor->GetComponentSnapshot()};

    for (UActorComponent* Component : Components) {
        if (!Component->IsBeingDestroyed() && Component->GetClass()->IsChildOf(USceneComponent::StaticClass())) {
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
    if (!mState.IsSet(EStateFlag::Initialized) || mState.IsSet(EStateFlag::Ticking) || mState.IsSet(EStateFlag::BeginningPlay) || mState.IsSet(EStateFlag::EndingPlay) || mState.IsSet(EStateFlag::FlushingActors) || mActorDispatchDepth > 0 || IsDeferringLifecycleChanges() || mState.IsSet(EStateFlag::ProcessingLifecycleRequests)) {
        return;
    }

    mState.Set(EStateFlag::FlushingActors);

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

    mState.Clear(EStateFlag::FlushingActors);

    if (RemovedAnyActor) {
        MarkStructureDirty();
    }
}

void UWorld::AttachActor(AActor* Child, AActor* Parent) {
    if (Child == nullptr || Parent == nullptr || Child == Parent || Child->GetWorld() != this || Parent->GetWorld() != this) {
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
}

void UWorld::DetachActor(AActor* Actor) {
    if (Actor == nullptr || Actor->GetWorld() != this) {
        return;
    }

    USceneComponent* RootComponent{Actor->GetRootComponent()};

    if (RootComponent == nullptr) {
        return;
    }

    RootComponent->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
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
    mSubsystems.Add<UOverlaySubsystem>();
}

void UWorld::DeinitializeSubsystems() {
    mSubsystems.Deinitialize();
}

UOverlaySubsystem& UWorld::GetOverlaySubsystem() {
    return *mSubsystems.Get<UOverlaySubsystem>();
}

const UOverlaySubsystem& UWorld::GetOverlaySubsystem() const {
    return *mSubsystems.Get<UOverlaySubsystem>();
}

void UWorld::BindScene(const std::weak_ptr<FSceneInterface>& Scene) {
    const std::shared_ptr<FSceneInterface> Previous{mScene.lock()};
    const std::shared_ptr<FSceneInterface> Next{Scene.lock()};

    if (Previous == Next) {
        return;
    }

    if (Previous != nullptr) {
        Previous->Release();
    }

    mScene = Scene;
    mAppliedSceneHandle = {};
    mSceneUpdates = {};
}

FSceneHandle UWorld::GetSceneHandle() const {
    const std::shared_ptr<FSceneInterface> Scene{mScene.lock()};

    return Scene != nullptr ? Scene->GetHandle() : FSceneHandle{};
}

bool UWorld::SendSceneUpdates() {
    const std::shared_ptr<FSceneInterface> Scene{mScene.lock()};

    if (!mState.IsSet(EStateFlag::Initialized) || Scene == nullptr) {
        return false;
    }

    const FSceneHandle Handle{Scene->GetHandle()};

    if (!Handle.IsValid()) {
        return false;
    }

    const bool FullSnapshot{mAppliedSceneHandle != Handle};

    if (FullSnapshot) {
        GetRenderSubsystem().RecreateRenderStates();
    }

    GetRenderSubsystem().BuildSceneUpdates(mSceneUpdates);
    mSceneUpdates.mSceneHandle = Handle;
    mSceneUpdates.mFullSnapshot = FullSnapshot;

    if (!Scene->ApplyUpdates(mSceneUpdates)) {
        mAppliedSceneHandle = {};
        return false;
    }

    mAppliedSceneHandle = Handle;

    return true;
}

void UWorld::BuildOverlayRenderData(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) {
    mSubsystems.Get<UOverlaySubsystem>()->BuildDrawData(Overlay, SelectedActor);
}

void UWorld::MarkStructureDirty() {
    ++mStructureRevision;
    NotifyWorldChanged(EWorldChange::StructureChanged);
}

Uint64 UWorld::GetStructureRevision() const {
    return mStructureRevision;
}

void UWorld::Tick(float DeltaTime) {
    Tick(GetDefaultTickType(), DeltaTime);
}

void UWorld::Tick(ETickType TickType, float DeltaTime) {
    if (!mState.IsSet(EStateFlag::Initialized) || IsBusy() || TickType == ETickType::None) {
        return;
    }

    if (mState.IsSet(EStateFlag::EndPlayRequested)) {
        EndPlay(mEndPlayReason);
    }

    const ETickType EffectiveTickType{mTime.IsPaused() && (TickType == ETickType::Game || TickType == ETickType::Editor) ? ETickType::Paused : TickType};

    if ((EffectiveTickType == ETickType::Game || EffectiveTickType == ETickType::Paused) && !mState.IsSet(EStateFlag::HasBegunPlay)) {
        FlushPendingDestroyActors();
        return;
    }

    mTime.Tick(EffectiveTickType == ETickType::Paused ? 0.0 : static_cast<double>(DeltaTime));

    const float WorldDeltaTime{static_cast<float>(mTime.GetDeltaSeconds())};
    const bool CanRunTicks{EffectiveTickType != ETickType::TimeOnly && (WorldDeltaTime > 0.0f || (EffectiveTickType == ETickType::Paused && std::isfinite(DeltaTime) && DeltaTime > 0.0f))};

    if (CanRunTicks) {
        const Stat::FScopedWorldTickStatTimer TickStat{0};
        Stat::FWorldTickStats* TickStats{Stat::GetActiveWorldTickStats()};
        const std::size_t ActorCount{mActorTickFunctions.size()};
        const std::size_t ComponentCount{mComponentTickFunctions.size()};

        mState.Set(EStateFlag::Ticking);

        for (std::size_t Index{}; Index < ActorCount && !mState.IsSet(EStateFlag::EndPlayRequested); ++Index) {
            FTickFunction* TickFunction{mActorTickFunctions[Index]};

            if (TickFunction == nullptr || !TickFunction->CanTick(EffectiveTickType)) {
                continue;
            }

            if (TickStats != nullptr) {
                ++TickStats->mActorTickCount;
            }

            TickFunction->ExecuteTick(WorldDeltaTime);
        }

        for (std::size_t Index{}; Index < ComponentCount && !mState.IsSet(EStateFlag::EndPlayRequested); ++Index) {
            FTickFunction* TickFunction{mComponentTickFunctions[Index]};

            if (TickFunction == nullptr || !TickFunction->CanTick(EffectiveTickType)) {
                continue;
            }

            if (TickStats != nullptr) {
                ++TickStats->mComponentVisitCount;
                ++TickStats->mComponentTickCount;
            }

            TickFunction->ExecuteTick(WorldDeltaTime);
        }

        FinishTicks();
    }

    if (mState.IsSet(EStateFlag::EndPlayRequested)) {
        EndPlay(mEndPlayReason);
    }

    FlushPendingDestroyActors();
}

ETickType UWorld::GetDefaultTickType() const {
    return IsGameWorld() ? ETickType::Game : ETickType::Editor;
}

bool UWorld::IsBusy() const {
    return mState.HasAny({EStateFlag::Ticking, EStateFlag::CleaningUp, EStateFlag::LoadingScene, EStateFlag::BeginningPlay, EStateFlag::EndingPlay, EStateFlag::FlushingActors, EStateFlag::ProcessingLifecycleRequests}) || mActorDispatchDepth > 0 || IsDeferringLifecycleChanges();
}

void UWorld::RegisterTickActor(AActor* Actor) {
    RegisterTickFunction(Actor->mPrimaryActorTick, mActorTickFunctions);
}

void UWorld::UnregisterTickActor(AActor* Actor) {
    UnregisterTickFunction(Actor->mPrimaryActorTick, mActorTickFunctions, EStateFlag::ActorTickFunctionsNeedCompaction);
}

void UWorld::RegisterTickComponent(UActorComponent* Component) {
    RegisterTickFunction(Component->mPrimaryComponentTick, mComponentTickFunctions);
}

void UWorld::UnregisterTickComponent(UActorComponent* Component) {
    UnregisterTickFunction(Component->mPrimaryComponentTick, mComponentTickFunctions, EStateFlag::ComponentTickFunctionsNeedCompaction);
}

void UWorld::RegisterTickFunction(FTickFunction& TickFunction, TArray<FTickFunction*>& TickFunctions) {
    if (TickFunction.mTickIndex != std::numeric_limits<std::size_t>::max()) {
        return;
    }

    TickFunctions.push_back(&TickFunction);
    TickFunction.mTickIndex = TickFunctions.size() - 1;
}

void UWorld::UnregisterTickFunction(FTickFunction& TickFunction, TArray<FTickFunction*>& TickFunctions, EStateFlag CompactionFlag) {
    const std::size_t Index{TickFunction.mTickIndex};

    if (Index == std::numeric_limits<std::size_t>::max()) {
        return;
    }

    TickFunction.mTickIndex = std::numeric_limits<std::size_t>::max();

    if (mState.IsSet(EStateFlag::Ticking)) {
        TickFunctions[Index] = nullptr;
        mState.Set(CompactionFlag);
    } else {
        if (Index + 1 < TickFunctions.size()) {
            FTickFunction* LastTickFunction{TickFunctions.back()};

            TickFunctions[Index] = LastTickFunction;
            LastTickFunction->mTickIndex = Index;
        }

        TickFunctions.pop_back();
    }
}

void UWorld::CompactTickFunctions(TArray<FTickFunction*>& TickFunctions, EStateFlag CompactionFlag) {
    if (!mState.IsSet(CompactionFlag)) {
        return;
    }

    std::erase(TickFunctions, nullptr);

    for (std::size_t Index{}; Index < TickFunctions.size(); ++Index) {
        TickFunctions[Index]->mTickIndex = Index;
    }

    mState.Clear(CompactionFlag);
}

void UWorld::FinishTicks() {
    mState.Clear(EStateFlag::Ticking);
    CompactTickFunctions(mActorTickFunctions, EStateFlag::ActorTickFunctionsNeedCompaction);
    CompactTickFunctions(mComponentTickFunctions, EStateFlag::ComponentTickFunctionsNeedCompaction);
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

AActor* UWorld::AddActorInternal(std::unique_ptr<AActor> InActor) {
    if (!mState.IsSet(EStateFlag::Initialized) || mState.IsSet(EStateFlag::CleaningUp) || mState.IsSet(EStateFlag::EndingPlay) || !InActor || InActor->GetWorld() != nullptr || InActor->IsBeingDestroyed()) {
        return nullptr;
    }

    const FActorDispatchScope Dispatch{this};
    AActor* Actor{InActor.get()};

    if (!Actor->SetOuter(mPersistentLevel.get()) || !UObjectSystem::Register(Actor).IsValid()) {
        return nullptr;
    }

    if (IsDeferringLifecycleChanges()) {
        Actor->mWorld = this;
        mPendingLifecycleRequests.push_back({ELifecycleRequestType::AddActor, Actor, std::move(InActor)});
        return Actor;
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

    if (!IsDeferringLifecycleChanges()) {
        Actor->PostActorCreated();
    }

    return FinishSpawningActor(Actor, Transform) ? Actor : nullptr;
}

AActor* UWorld::SpawnActor(const UClass& Type, const FTransform& Transform) {
    const FActorDispatchScope Dispatch{this};
    AActor* Actor{SpawnActorDeferred(Type, Transform)};

    return Actor != nullptr && FinishSpawningActor(Actor, Transform) ? Actor : nullptr;
}

AActor* UWorld::SpawnActorDeferred(const UClass& Type, const FTransform& Transform) {
    if (!mState.IsSet(EStateFlag::Initialized) || mState.IsSet(EStateFlag::CleaningUp) || mState.IsSet(EStateFlag::EndingPlay) || !Type.CanCreateInstance() || !Type.IsChildOf(AActor::StaticClass())) {
        return nullptr;
    }

    const FActorDispatchScope Dispatch{this};
    std::unique_ptr<UObject> Object{Type.CreateInstance()};

    if (Object == nullptr || !Object->GetClass()->IsChildOf(AActor::StaticClass())) {
        return nullptr;
    }

    std::unique_ptr<AActor> Owner{static_cast<AActor*>(Object.release())};
    AActor* Actor{AddActorInternal(std::move(Owner))};

    if (Actor == nullptr) {
        return nullptr;
    }

    Actor->SetActorTransform(Transform);
    if (!IsDeferringLifecycleChanges()) {
        Actor->PostActorCreated();
    }

    return Actor->IsBeingDestroyed() ? nullptr : Actor;
}

bool UWorld::FinishSpawningActor(AActor* Actor, const FTransform& Transform) {
    if (Actor == nullptr || Actor->GetWorld() != this || Actor->mState.IsSet(AActor::EStateFlag::FinishedSpawning) || Actor->mState.IsSet(AActor::EStateFlag::FinishingSpawning) || Actor->IsBeingDestroyed()) {
        return false;
    }

    if (IsDeferringLifecycleChanges()) {
        Actor->mState.Set(AActor::EStateFlag::FinishingSpawning);
        mPendingLifecycleRequests.push_back({ELifecycleRequestType::FinishSpawningActor, Actor, {}, Transform});
        return true;
    }

    const FActorDispatchScope Dispatch{this};
    const bool HadRootComponent{Actor->GetRootComponent() != nullptr};

    Actor->mState.Set(AActor::EStateFlag::FinishingSpawning);
    Actor->SetActorTransform(Transform);
    Actor->OnConstruction(Transform);

    if (!HadRootComponent) {
        Actor->SetActorTransform(Transform);
    }

    Actor->mState.Clear(AActor::EStateFlag::FinishingSpawning);

    if (Actor->IsBeingDestroyed()) {
        return false;
    }

    Actor->mState.Set(AActor::EStateFlag::FinishedSpawning);
    Actor->RegisterAllComponents();

    if (mState.IsSet(EStateFlag::HasBegunPlay) && !mState.IsSet(EStateFlag::LoadingScene)) {
        Actor->DispatchBeginPlay();
    } else if (mState.IsSet(EStateFlag::BeginningPlay) && !mState.IsSet(EStateFlag::LoadingScene)) {
        Actor->InitializeComponents();
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

    Actor.mState.Set(AActor::EStateFlag::FinishedSpawning);
    Actor.RegisterAllComponents();

    if (mState.IsSet(EStateFlag::HasBegunPlay) && !mState.IsSet(EStateFlag::LoadingScene)) {
        Actor.DispatchBeginPlay();
    }

    if (!Actor.IsBeingDestroyed()) {
        NotifyWorldChanged(EWorldChange::ActorAdded, &Actor);
        MarkStructureDirty();
    }
}

void UWorld::ClearActors() {
    mPendingDestroyActors.reserve(mPersistentLevel->mActors.size());

    const std::size_t Count{mPersistentLevel->mActors.size()};

    for (std::size_t Index{}; Index < Count; ++Index) {
        AActor* Actor{mPersistentLevel->mActors[Index].get()};

        if (Actor->GetWorld() == this) {
            DestroyActorInternal(Actor, mState.IsSet(EStateFlag::CleaningUp) ? EEndPlayReason::Quit : EEndPlayReason::LevelTransition);
        } else if (!Actor->mState.IsSet(AActor::EStateFlag::BeingDestroyed)) {
            Actor->mState.Set(AActor::EStateFlag::BeingDestroyed);
            mPendingDestroyActors.push_back(Actor);
        }
    }

    FlushPendingDestroyActors();
}

FName UWorld::MakeUniqueObjectName(FName SourceName) {
    return mState.IsSet(EStateFlag::Initialized) ? UObjectSystem::MakeUniqueObjectName(mPersistentLevel.get(), SourceName) : FName{};
}

AActor* UWorld::FindActorByName(FName InName) const {
    UObject* Object{mState.IsSet(EStateFlag::Initialized) ? UObjectSystem::FindObject(mPersistentLevel.get(), InName) : nullptr};

    AActor* Actor{Object != nullptr && Object->GetClass()->IsChildOf(AActor::StaticClass()) ? static_cast<AActor*>(Object) : nullptr};

    return Actor != nullptr && !Actor->IsBeingDestroyed() && std::ranges::find(mPersistentLevel->mActors, Actor, &std::unique_ptr<AActor>::get) != mPersistentLevel->mActors.end() ? Actor : nullptr;
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
