#include "pch.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "CoreUObject/Reflection/UClass.h"
#include "World/AActor.h"
#include "Core/Stat/Stat.h"
#include "World/UWorld.h"
#include "World/Component/USceneComponent.h"

const std::vector<std::unique_ptr<UActorComponent>>& AActor::GetComponents() const {
    return mComponents;
}

AActor::~AActor() {
    mBIsBeingDestroyed = true;
    SetWorld(nullptr);

    while (!mComponents.empty()) {
        mComponents.back()->DestroyComponent();
    }
}

UActorComponent* AActor::AddComponent(const UClass& Type) {
    if (mBIsBeingDestroyed || !Type.CanCreateInstance() || !Type.IsChildOf(UActorComponent::StaticClass())) {
        return nullptr;
    }

    const UWorld::FActorDispatchScope Dispatch{mWorld};
    std::unique_ptr<UObject> CreatedObject{Type.CreateInstance()};

    if (CreatedObject == nullptr ||
        !CreatedObject->GetClass()->IsChildOf(UActorComponent::StaticClass())) {
        return nullptr;
    }

    std::unique_ptr<UActorComponent> NewComponent{static_cast<UActorComponent*>(CreatedObject.release())};
    UActorComponent* ComponentPtr{NewComponent.get()};

    ComponentPtr->SetOwner(this);
    if (!UObjectSystem::Register(ComponentPtr).IsValid()) {
        return nullptr;
    }
    mComponents.push_back(std::move(NewComponent));

    FinishAddingComponent(*ComponentPtr);

    if (mWorld != nullptr && mHasFinishedSpawning && !mBIsBeingDestroyed) {
        mWorld->MarkStructureDirty();
    }

    return ComponentPtr->IsBeingDestroyed() || mBIsBeingDestroyed ? nullptr : ComponentPtr;
}

void AActor::RemoveOwnedComponent(UActorComponent* Component) {
    const auto Iterator{std::ranges::find_if(mComponents, [Component](const std::unique_ptr<UActorComponent>& Candidate) {
        return Candidate.get() == Component;
    })};

    if (Iterator == mComponents.end()) {
        return;
    }

    if (mRootComponent == Component) {
        mRootComponent = nullptr;
    }

    UObjectSystem::Unregister(Component, Component->GetHandle());
    mPendingDestroyComponents.push_back(std::move(*Iterator));
    mComponents.erase(Iterator);

    if (mWorld != nullptr && !mBIsBeingDestroyed) {
        mWorld->MarkStructureDirty();
    }
}

USceneComponent* AActor::GetRootComponent() {
    return mRootComponent;
}

const USceneComponent* AActor::GetRootComponent() const {
    return mRootComponent;
}

bool AActor::SetRootComponent(USceneComponent* InRootComponent) {
    if (mBIsBeingDestroyed || (InRootComponent != nullptr && InRootComponent->IsBeingDestroyed())) {
        return false;
    }

    if (InRootComponent != nullptr) {
        const bool BIsOwnedComponent{std::ranges::any_of(mComponents, [InRootComponent](const std::unique_ptr<UActorComponent>& Component) {
            return Component.get() == InRootComponent;
        })};

        if (!BIsOwnedComponent) {
            return false;
        }
    }

    mRootComponent = InRootComponent;
    mPendingRootComponentGuid = {};

    return true;
}

void AActor::SetWorld(UWorld* InWorld) {
    if (mWorld == InWorld) {
        return;
    }

    const UWorld::FActorDispatchScope PreviousDispatch{mWorld};
    const UWorld::FActorDispatchScope NextDispatch{InWorld};

    if (mWorld != nullptr) {
        DispatchEndPlay(EEndPlayReason::RemovedFromWorld);
        mWorld->UnregisterTickActor(this);

        for (UActorComponent* Component : GetComponentSnapshot()) {
            Component->UnregisterComponent();
        }

        OnRemovedFromWorld();
        mWorld = nullptr;
    }

    if (InWorld != nullptr && !mBIsBeingDestroyed) {
        mWorld = InWorld;
        OnAddedToWorld();
    }
}

UWorld* AActor::GetWorld() const {
    return mWorld;
}

ULevel* AActor::GetLevel() const {
    return GetTypedOuter<ULevel>();
}

void AActor::FinishAddingComponent(UActorComponent& Component) {
    if (mWorld != nullptr && mHasFinishedSpawning && !mBIsBeingDestroyed) {
        Component.RegisterComponent(mWorld);
    }
}

void AActor::OnComponentRegistered(UActorComponent& Component) {
    if (mActorInitialized) {
        AdvanceComponentLifecycle(Component);
    } else {
        QueueComponentInitialization(Component);
    }
}

void AActor::QueueComponentInitialization(UActorComponent& Component) {
    if (!mActorInitialized && mLifecyclePhase == ELifecyclePhase::InitializingComponents && Component.IsRegistered() && !Component.IsInitialized() && !Component.IsBeingDestroyed()) {
        mPendingInitializeComponents.push_back(&Component);
    }
}

void AActor::AdvanceComponentLifecycle(UActorComponent& Component) {
    if (!CanAdvanceComponentLifecycle() || !Component.IsRegistered() || Component.IsBeingDestroyed()) {
        return;
    }

    Component.DispatchInitializeComponent();

    if (mBHasBegunPlay && CanAdvanceComponentLifecycle()) {
        Component.DispatchBeginPlay();
    }
}

bool AActor::CanAdvanceComponentLifecycle() const {
    return mWorld != nullptr && !mBIsBeingDestroyed && !mEndPlayRequested && !mWorld->mEndPlayRequested && mLifecyclePhase != ELifecyclePhase::EndingPlay;
}

TArray<UActorComponent*> AActor::GetComponentSnapshot() const {
    TArray<UActorComponent*> Components{};

    Components.reserve(mComponents.size());

    for (const std::unique_ptr<UActorComponent>& Component : mComponents) {
        Components.push_back(Component.get());
    }

    return Components;
}

void AActor::RegisterAllComponents() {
    const UWorld::FActorDispatchScope Dispatch{mWorld};

    for (UActorComponent* Component : GetComponentSnapshot()) {
        if (mBIsBeingDestroyed) {
            break;
        }

        if (!Component->IsBeingDestroyed()) {
            Component->RegisterComponent(mWorld);
        }
    }

    UpdateTickRegistration();
}

void AActor::DispatchBeginPlay() {
    if (!CanAdvanceComponentLifecycle() || !mWorld->HasBegunPlay() || mBHasBegunPlay || mLifecyclePhase != ELifecyclePhase::None || !mHasFinishedSpawning) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mWorld};

    InitializeComponents();

    if (!CanAdvanceComponentLifecycle() || !mWorld->HasBegunPlay() || !mActorInitialized) {
        return;
    }

    mBHasBegunPlay = true;
    mLifecyclePhase = ELifecyclePhase::BeginningPlay;

    for (UActorComponent* Component : GetComponentSnapshot()) {
        if (!CanAdvanceComponentLifecycle()) {
            break;
        }

        AdvanceComponentLifecycle(*Component);
    }

    if (CanAdvanceComponentLifecycle()) {
        BeginPlay();
    }

    mLifecyclePhase = ELifecyclePhase::None;
    UpdateTickRegistration();

    if (mEndPlayRequested) {
        DispatchEndPlay(mEndPlayReason);
    }
}

void AActor::DispatchEndPlay(EEndPlayReason Reason) {
    if (mLifecyclePhase == ELifecyclePhase::EndingPlay) {
        return;
    }

    if (mLifecyclePhase != ELifecyclePhase::None) {
        mEndPlayRequested = true;
        mEndPlayReason = Reason;
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mWorld};
    const bool HadBegunPlay{mBHasBegunPlay};

    mLifecyclePhase = ELifecyclePhase::EndingPlay;
    mEndPlayRequested = false;
    mBHasBegunPlay = false;
    mActorInitialized = false;
    UpdateTickRegistration();

    if (HadBegunPlay) {
        EndPlay(Reason);
    }

    for (UActorComponent* Component : GetComponentSnapshot()) {
        Component->DispatchEndPlay(Reason);
        Component->DispatchUninitializeComponent();
    }

    mLifecyclePhase = ELifecyclePhase::None;
}

bool AActor::Destroy() {
    return mWorld != nullptr && mWorld->DestroyActor(this);
}

FTransform AActor::GetActorTransform() const {
    if (mRootComponent == nullptr) {
        return {};
    }

    return mRootComponent->GetComponentTransform();
}

bool AActor::SetActorTransform(const FTransform& Transform) {
    if (mRootComponent == nullptr) {
        return false;
    }

    return mRootComponent->SetWorldTransform(Transform);
}

FVector3 AActor::GetActorLocation() const {
    if (mRootComponent == nullptr) {
        return FVector3::Zero;
    }

    return mRootComponent->GetComponentLocation();
}

bool AActor::SetActorLocation(const FVector3& Location) {
    if (mRootComponent == nullptr) {
        return false;
    }

    return mRootComponent->SetWorldLocation(Location);
}

bool AActor::SetActorLocationAndRotation(const FVector3& Location, const FRotator& Rotation) {
    return mRootComponent != nullptr && mRootComponent->SetWorldLocationAndRotation(Location, Rotation);
}

FRotator AActor::GetActorRotation() const {
    if (mRootComponent == nullptr) {
        return FRotator{};
    }

    return mRootComponent->GetComponentRotation();
}

bool AActor::SetActorRotation(const FRotator& Rotation) {
    return mRootComponent != nullptr && mRootComponent->SetWorldRotation(Rotation);
}

FVector3 AActor::GetActorScale3D() const {
    if (mRootComponent == nullptr) {
        return {1.0f, 1.0f, 1.0f};
    }

    return mRootComponent->GetComponentScale();
}

bool AActor::SetActorScale3D(const FVector3& Scale) {
    return mRootComponent != nullptr && mRootComponent->SetWorldScale3D(Scale);
}

FTransform AActor::GetActorRelativeTransform() const {
    return mRootComponent != nullptr ? mRootComponent->GetRelativeTransform() : FTransform{};
}

bool AActor::SetActorRelativeTransform(const FTransform& Transform) {
    if (mRootComponent == nullptr) {
        return false;
    }

    mRootComponent->SetRelativeTransform(Transform);

    return true;
}

FVector3 AActor::GetActorRelativeLocation() const {
    return mRootComponent != nullptr ? mRootComponent->GetRelativeLocation() : FVector3::Zero;
}

bool AActor::SetActorRelativeLocation(const FVector3& Location) {
    if (mRootComponent == nullptr) {
        return false;
    }

    mRootComponent->SetRelativeLocation(Location);

    return true;
}

bool AActor::SetActorRelativeLocationAndRotation(const FVector3& Location, const FRotator& Rotation) {
    if (mRootComponent == nullptr) {
        return false;
    }

    mRootComponent->SetRelativeLocationAndRotation(Location, Rotation);

    return true;
}

FRotator AActor::GetActorRelativeRotation() const {
    return mRootComponent != nullptr ? mRootComponent->GetRelativeRotation() : FRotator{};
}

bool AActor::SetActorRelativeRotation(const FRotator& Rotation) {
    if (mRootComponent == nullptr) {
        return false;
    }

    mRootComponent->SetRelativeRotation(Rotation);

    return true;
}

FVector3 AActor::GetActorRelativeScale3D() const {
    return mRootComponent != nullptr ? mRootComponent->GetRelativeScale3D() : FVector3{1.0f, 1.0f, 1.0f};
}

bool AActor::SetActorRelativeScale3D(const FVector3& Scale) {
    if (mRootComponent == nullptr) {
        return false;
    }

    mRootComponent->SetRelativeScale3D(Scale);

    return true;
}

bool AActor::HasBegunPlay() const {
    return mBHasBegunPlay;
}

bool AActor::IsBeingDestroyed() const {
    return mBIsBeingDestroyed;
}

bool AActor::HasFinishedSpawning() const {
    return mHasFinishedSpawning;
}

bool AActor::CanEverTick() const {
    return mPrimaryActorTick.CanEverTick();
}

void AActor::SetCanEverTick(bool CanEverTick) {
    mPrimaryActorTick.SetCanEverTick(CanEverTick);
    UpdateTickRegistration();
}

bool AActor::IsTickEnabled() const {
    return mPrimaryActorTick.IsTickEnabled();
}

void AActor::SetTickEnabled(bool TickEnabled) {
    if (mPrimaryActorTick.IsTickEnabled() == TickEnabled) {
        return;
    }

    mPrimaryActorTick.SetTickEnabled(TickEnabled);
    UpdateTickRegistration();
}

bool AActor::IsTickInEditor() const {
    return mPrimaryActorTick.IsTickInEditor();
}

void AActor::SetTickInEditor(bool TickInEditor) {
    mPrimaryActorTick.SetTickInEditor(TickInEditor);
    UpdateTickRegistration();
}

bool AActor::IsTickWhenPaused() const {
    return mPrimaryActorTick.IsTickWhenPaused();
}

void AActor::SetTickWhenPaused(bool TickWhenPaused) {
    mPrimaryActorTick.SetTickWhenPaused(TickWhenPaused);
}

const FActorTickFunction& AActor::GetTickFunction() const {
    return mPrimaryActorTick;
}

void AActor::UpdateTickRegistration() {
    if (mWorld == nullptr) {
        return;
    }

    if (mPrimaryActorTick.CanTick(mWorld->GetDefaultTickType())) {
        mWorld->RegisterTickActor(this);
    } else {
        mWorld->UnregisterTickActor(this);
    }
}

void AActor::Tick(float DeltaTime) {
}

void AActor::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    // components
    std::size_t ArraySize{mComponents.size()};

    Archive.BeginArrayScope("Components", ArraySize);

    if (Archive.HasError() || (Archive.IsLoading() && ArraySize != mComponents.size())) {
        Archive.SetError();
        Archive.EndArrayScope();
        return;
    }

    for (std::size_t I{0}; I < ArraySize && !Archive.HasError(); ++I) {
        Archive.BeginObjectScope(std::to_string(I));
        mComponents[I]->Serialize(Archive);
        Archive.EndObjectScope();
    }

    Archive.EndArrayScope();

}

void AActor::OnAddedToWorld() {
}

void AActor::InitializeComponents() {
    if (mActorInitialized || mLifecyclePhase != ELifecyclePhase::None || !CanAdvanceComponentLifecycle() || !mHasFinishedSpawning) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mWorld};

    mLifecyclePhase = ELifecyclePhase::InitializingComponents;
    mPendingInitializeComponents = GetComponentSnapshot();
    PreInitializeComponents();

    for (std::size_t Index{}; Index < mPendingInitializeComponents.size() && CanAdvanceComponentLifecycle(); ++Index) {
        UActorComponent* Component{mPendingInitializeComponents[Index]};

        AdvanceComponentLifecycle(*Component);
    }

    mPendingInitializeComponents.clear();

    if (CanAdvanceComponentLifecycle()) {
        mActorInitialized = true;
        PostInitializeComponents();
    }

    mLifecyclePhase = ELifecyclePhase::None;

    if (mEndPlayRequested) {
        DispatchEndPlay(mEndPlayReason);
    }
}

void AActor::BeginPlay() {
}

void AActor::EndPlay(EEndPlayReason Reason) {
}

void AActor::PostActorCreated() {
}

void AActor::PostLoad() {
}

void AActor::OnConstruction(const FTransform& Transform) {
}

void AActor::PreInitializeComponents() {
}

void AActor::PostInitializeComponents() {
}

void AActor::Destroyed() {
}

void AActor::OnRemovedFromWorld() {
}

bool AActor::ResolveLoadedReferences() {
    if (mPendingRootComponentGuid.IsValid()) {
        UObject* ResolvedObject{UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(mPendingRootComponentGuid))};

        if (ResolvedObject == nullptr ||
            !ResolvedObject->GetClass()->IsChildOf(USceneComponent::StaticClass()) || static_cast<USceneComponent*>(ResolvedObject)->GetOwner() != this) {
            return false;
        }

        mRootComponent = static_cast<USceneComponent*>(ResolvedObject);
        mPendingRootComponentGuid = {};
    }

    for (const std::unique_ptr<UActorComponent>& Component : mComponents) {
        if (!Component->ResolveLoadedReferences()) {
            return false;
        }
    }

    return true;
}

bool AActor::CanChangeOuter(const UObject* NewOuter) const {
    if (mWorld != nullptr) {
        return NewOuter == &mWorld->GetPersistentLevel();
    }

    return NewOuter == nullptr || NewOuter->GetClass()->IsChildOf(ULevel::StaticClass());
}

void AActor::OnIdentityChanged() {
    if (mWorld != nullptr && !mBIsBeingDestroyed) {
        mWorld->MarkStructureDirty();
    }
}

AActor::AActor() {
    InitializeClass(StaticClass());
}

void AActor::SerializeRootComponentProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    AActor& Object{*static_cast<AActor*>(Container)};

    SerializeObjectReference(Archive, Property.GetSerializationName(), Object.mRootComponent, Object.mPendingRootComponentGuid);

    if (Archive.IsLoading()) {
        Object.mRootComponent = {};
    }
}
