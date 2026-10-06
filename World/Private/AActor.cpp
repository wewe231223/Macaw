#include "pch.h"
#include "World/AActor.h"
#include "Core/Stat/Stat.h"
#include "World/UWorld.h"
#include "World/Component/USceneComponent.h"
#include "CoreUObject/TypeRegistry.h"

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

UActorComponent* AActor::AddComponent(const FTypeInfo& Type) {
    if (mBIsBeingDestroyed || Type.mCreator == nullptr || !Type.IsA(UActorComponent::StaticTypeInfo())) {
        return nullptr;
    }

    const UWorld::FActorDispatchScope Dispatch{mWorld};
    std::unique_ptr<UObject> CreatedObject{Type.mCreator()};

    if (CreatedObject == nullptr ||
        !CreatedObject->GetTypeInfo()->IsA(UActorComponent::StaticTypeInfo())) {
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
    if (mWorld == nullptr || !mWorld->HasBegunPlay() || mWorld->mEndPlayRequested || mBHasBegunPlay || mBeginningPlay || mEndingPlay || mBIsBeingDestroyed || !mHasFinishedSpawning) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mWorld};

    InitializeComponents();

    if (mBIsBeingDestroyed || !mWorld->HasBegunPlay() || mWorld->mEndPlayRequested || !mActorInitialized) {
        return;
    }

    mBeginningPlay = true;
    mBHasBegunPlay = true;

    for (UActorComponent* Component : GetComponentSnapshot()) {
        if (mBIsBeingDestroyed || mEndPlayRequested) {
            break;
        }

        Component->DispatchBeginPlay();
    }

    if (!mBIsBeingDestroyed && !mEndPlayRequested && !mWorld->mEndPlayRequested) {
        BeginPlay();
    }

    mBeginningPlay = false;
    UpdateTickRegistration();

    if (mEndPlayRequested) {
        DispatchEndPlay(mEndPlayReason);
    }
}

void AActor::DispatchEndPlay(EEndPlayReason Reason) {
    if (mEndingPlay) {
        return;
    }

    if (mBeginningPlay || mInitializingComponents) {
        mEndPlayRequested = true;
        mEndPlayReason = Reason;
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mWorld};
    const bool HadBegunPlay{mBHasBegunPlay};

    mEndingPlay = true;
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

    mEndingPlay = false;
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
        return FRotator::Zero;
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
    return mRootComponent != nullptr ? mRootComponent->GetRelativeRotation() : FRotator::Zero;
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
    return mCanEverTick;
}

void AActor::SetCanEverTick(bool CanEverTick) {
    mCanEverTick = CanEverTick;
    UpdateTickRegistration();
}

bool AActor::IsTickEnabled() const {
    return mBTickEnabled;
}

void AActor::SetTickEnabled(bool TickEnabled) {
    if (mBTickEnabled == TickEnabled) {
        return;
    }

    mBTickEnabled = TickEnabled;
    UpdateTickRegistration();
}

bool AActor::IsTickInEditor() const {
    return mTickInEditor;
}

void AActor::SetTickInEditor(bool TickInEditor) {
    mTickInEditor = TickInEditor;
    UpdateTickRegistration();
}

void AActor::UpdateTickRegistration() {
    if (mWorld == nullptr) {
        return;
    }

    if (mCanEverTick && mBTickEnabled && !mBIsBeingDestroyed && mHasFinishedSpawning && (mBHasBegunPlay || (mTickInEditor && mWorld->GetWorldType() != EWorldType::Game))) {
        mWorld->RegisterTickActor(this);
    } else {
        mWorld->UnregisterTickActor(this);
    }
}

void AActor::Tick(float DeltaTime) {
}

void AActor::Serialize(FArchive& Archive) {
    UObject::Serialize(Archive);

    // components
    std::size_t ArraySize{mComponents.size()};

    Archive.BeginArrayScope("Components", ArraySize);

    for (std::size_t I{0}; I < ArraySize; ++I) {
        Archive.BeginObjectScope(std::to_string(I));
        mComponents[I]->Serialize(Archive);
        Archive.EndObjectScope();
    }

    Archive.EndArrayScope();

    // root component
    FString GuidRootComponent{};

    if (mRootComponent != nullptr) {
        GuidRootComponent = mRootComponent->GetGuid().ToString();
    }

    Archive.Serialize("GuidRootComponent", GuidRootComponent);

    if (Archive.IsLoading()) {
        mRootComponent = nullptr;
        mPendingRootComponentGuid = {};

        if (!GuidRootComponent.empty() && !mPendingRootComponentGuid.Parse(GuidRootComponent)) {
            mPendingRootComponentGuid = {};
        }
    }
}

void AActor::OnAddedToWorld() {
}

void AActor::InitializeComponents() {
    if (mActorInitialized || mInitializingComponents || mBIsBeingDestroyed || !mHasFinishedSpawning) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mWorld};

    mInitializingComponents = true;
    PreInitializeComponents();

    while (!mBIsBeingDestroyed && !mEndPlayRequested && !mWorld->mEndPlayRequested) {
        const auto Pending{std::ranges::find_if(mComponents, [](const std::unique_ptr<UActorComponent>& Component) {
            return Component->mWantsInitializeComponent && Component->IsRegistered() && !Component->IsInitialized() && !Component->IsBeingDestroyed();
        })};

        if (Pending == mComponents.end()) {
            break;
        }

        (*Pending)->DispatchInitializeComponent();
    }

    if (!mBIsBeingDestroyed && !mEndPlayRequested && !mWorld->mEndPlayRequested) {
        mActorInitialized = true;
        PostInitializeComponents();
    }

    mInitializingComponents = false;

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

bool AActor::PreLoadComponents(FArchive& Archive, bool RegisterComponents) {
    if (mWorld != nullptr || mBIsBeingDestroyed) {
        return false;
    }

    std::size_t ArraySize{};

    Archive.BeginArrayScope("Components", ArraySize);

    for (const std::unique_ptr<UActorComponent>& Component : mComponents) {
        Component->UnregisterComponent();
        UObjectSystem::Unregister(Component.get(), Component->GetHandle());
    }

    mRootComponent = nullptr;
    UpdateTickRegistration();
    mComponents.clear();
    mComponents.reserve(ArraySize);

    for (std::size_t I{0}; I < ArraySize; ++I) {
        Archive.BeginObjectScope(std::to_string(I));

        FName TypeName{};

        Archive.Serialize("TypeName", TypeName);

        const FTypeInfo* Type{TypeRegistry::Find(TypeName)};

        if (Type == nullptr || Type->mCreator == nullptr) {
            Archive.EndObjectScope();
            Archive.EndArrayScope();
            return false;
        }

        std::unique_ptr<UObject> CreatedObject{Type->mCreator()};

        if (CreatedObject == nullptr || !CreatedObject->GetTypeInfo()->IsA(UActorComponent::StaticTypeInfo())) {
            Archive.EndObjectScope();
            Archive.EndArrayScope();
            return false;
        }

        std::unique_ptr<UActorComponent> Component{static_cast<UActorComponent*>(CreatedObject.release())};

        Component->SetOwner(this);

        FGuid ComponentGuid{};

        Archive.Serialize("Guid", ComponentGuid);

        if (!Component->RestoreGuid(ComponentGuid) || (RegisterComponents && !UObjectSystem::Register(Component.get()).IsValid())) {
            Archive.EndObjectScope();
            Archive.EndArrayScope();
            return false;
        }

        mComponents.push_back(std::move(Component));

        Archive.EndObjectScope();
    }

    Archive.EndArrayScope();

    return true;
}

bool AActor::ResolveLoadedReferences() {
    if (mPendingRootComponentGuid.IsValid()) {
        UObject* ResolvedObject{UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(mPendingRootComponentGuid))};

        if (ResolvedObject == nullptr ||
            !ResolvedObject->GetTypeInfo()->IsA(USceneComponent::StaticTypeInfo()) || static_cast<USceneComponent*>(ResolvedObject)->GetOwner() != this) {
            return false;
        }

        mRootComponent = static_cast<USceneComponent*>(ResolvedObject);
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

    return NewOuter == nullptr || NewOuter->GetTypeInfo()->IsA(ULevel::StaticTypeInfo());
}

void AActor::OnIdentityChanged() {
    if (mWorld != nullptr && !mBIsBeingDestroyed) {
        mWorld->MarkStructureDirty();
    }
}
