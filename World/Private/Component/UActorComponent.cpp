#include "pch.h"
#include "World/Component/UActorComponent.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include "Core/Base/ErrorHandler.h"

AActor* UActorComponent::GetOwner() const {
    return mOwner;
}

void UActorComponent::SetOwner(AActor* InOwner) {
    if (mOwner == InOwner) {
        return;
    }

    if (mParentWorld != nullptr) {
        mParentWorld->UnregisterTickComponent(this);
    }

    mOwner = InOwner;
    SetOuter(InOwner);
    UpdateTickRegistration();
}

void UActorComponent::OnRegister() {
}

void UActorComponent::InitializeComponent() {
}

void UActorComponent::UninitializeComponent() {
}

void UActorComponent::BeginPlay() {
}

void UActorComponent::EndPlay(EEndPlayReason Reason) {
}

void UActorComponent::TickComponent(float DeltaTime) {
}

void UActorComponent::OnUnregister() {
}

void UActorComponent::OnRenderStateChanged() {
}

bool UActorComponent::IsActive() const {
    return mBActive;
}

void UActorComponent::SetActive(bool BInActive) {
    if (mBActive == BInActive || mBIsBeingDestroyed) {
        return;
    }

    if (BInActive) {
        Activate();
    } else {
        Deactivate();
    }
}

void UActorComponent::Activate() {
    if (mBIsBeingDestroyed) {
        return;
    }

    mBActive = true;
    SetTickEnabled(true);
}

void UActorComponent::Deactivate() {
    mBActive = false;
    SetTickEnabled(false);
}

bool UActorComponent::CanEverTick() const {
    return mCanEverTick;
}

void UActorComponent::SetCanEverTick(bool CanEverTick) {
    mCanEverTick = CanEverTick;
    UpdateTickRegistration();
}

bool UActorComponent::IsTickEnabled() const {
    return mBTickEnabled;
}

void UActorComponent::SetTickEnabled(bool TickEnabled) {
    mBTickEnabled = TickEnabled;
    UpdateTickRegistration();
}

bool UActorComponent::IsTickInEditor() const {
    return mTickInEditor;
}

void UActorComponent::SetTickInEditor(bool TickInEditor) {
    mTickInEditor = TickInEditor;
    UpdateTickRegistration();
}

void UActorComponent::SetWantsInitializeComponent(bool WantsInitializeComponent) {
    mWantsInitializeComponent = WantsInitializeComponent;
}

bool UActorComponent::IsBeingDestroyed() const {
    return mBIsBeingDestroyed;
}

void UActorComponent::UpdateTickRegistration() {
    if (mParentWorld == nullptr) {
        return;
    }

    const bool CanTick{mCanEverTick && mBTickEnabled && mBRegistered && !mBIsBeingDestroyed && mOwner != nullptr && !mOwner->IsBeingDestroyed() && mOwner->mHasFinishedSpawning && (mBHasBegunPlay || (mTickInEditor && mParentWorld->GetWorldType() != EWorldType::Game))};

    if (CanTick) {
        mParentWorld->RegisterTickComponent(this);
    } else {
        mParentWorld->UnregisterTickComponent(this);
    }
}

bool UActorComponent::IsRegistered() const {
    return mBRegistered;
}

bool UActorComponent::IsInitialized() const {
    return mBInitialized;
}

bool UActorComponent::HasBegunPlay() const {
    return mBHasBegunPlay;
}

UWorld* UActorComponent::GetBelongingWorld() const {
    return mParentWorld;
}

void UActorComponent::RegisterComponent(UWorld* World) {
    if (mBRegistered || mRegistering || mUnregistering || mBIsBeingDestroyed || (mOwner != nullptr && mOwner->IsBeingDestroyed())) {
        return;
    }

    ErrorHandler::Report(mOwner == nullptr || World == nullptr, "UActorComponent", "Component registration requires an owner and a world", ErrorHandler::EErrorLevel::Critical);
    ErrorHandler::Report(World != mOwner->GetWorld(), "UActorComponent", "Component world must match its owner", ErrorHandler::EErrorLevel::Critical);

    const UWorld::FActorDispatchScope Dispatch{World};

    mParentWorld = World;
    mBRegistered = true;
    mRegistering = true;
    OnRegister();
    mRegistering = false;

    if (mOwner->mActorInitialized) {
        DispatchInitializeComponent();
    }

    if (mOwner->HasBegunPlay()) {
        DispatchBeginPlay();
    }

    UpdateTickRegistration();
}

void UActorComponent::UnregisterComponent() {
    if (!mBRegistered || mUnregistering) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mParentWorld};

    mUnregistering = true;
    mBRegistered = false;
    UpdateTickRegistration();
    OnUnregister();
    mParentWorld = nullptr;
    mUnregistering = false;
}

void UActorComponent::DispatchInitializeComponent() {
    if (!mWantsInitializeComponent || mBInitialized || !mBRegistered || mBIsBeingDestroyed || mOwner->IsBeingDestroyed()) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mParentWorld};

    mBInitialized = true;
    InitializeComponent();
}

void UActorComponent::DispatchUninitializeComponent() {
    if (!mBInitialized) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mOwner != nullptr ? mOwner->GetWorld() : nullptr};

    mBInitialized = false;
    UninitializeComponent();
}

void UActorComponent::DispatchBeginPlay() {
    if (!mBRegistered || mBHasBegunPlay || mBIsBeingDestroyed || mOwner == nullptr || mOwner->IsBeingDestroyed() || !mOwner->HasBegunPlay() || mOwner->GetWorld()->mEndPlayRequested) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mParentWorld};

    DispatchInitializeComponent();

    if (!mBRegistered || mBIsBeingDestroyed || mOwner->IsBeingDestroyed() || !mOwner->HasBegunPlay() || mOwner->GetWorld()->mEndPlayRequested) {
        return;
    }

    mBHasBegunPlay = true;
    BeginPlay();
    UpdateTickRegistration();
}

void UActorComponent::DispatchEndPlay(EEndPlayReason Reason) {
    if (!mBHasBegunPlay) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mOwner != nullptr ? mOwner->GetWorld() : nullptr};

    mBHasBegunPlay = false;
    UpdateTickRegistration();
    EndPlay(Reason);
}

void UActorComponent::DestroyComponent(bool BPromoteChildren) {
    if (mBIsBeingDestroyed) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mOwner != nullptr ? mOwner->GetWorld() : nullptr};

    mBIsBeingDestroyed = true;
    DispatchEndPlay(EEndPlayReason::Destroyed);
    DispatchUninitializeComponent();
    UnregisterComponent();

    if (mOwner != nullptr) {
        mOwner->RemoveOwnedComponent(this);
    }
}

bool UActorComponent::ResolveLoadedReferences() {
    return true;
}

void UActorComponent::Serialize(FArchive& Archive) {
    UObject::Serialize(Archive);

    Archive.Serialize("bActive", mBActive);

    if (Archive.IsLoading()) {
        if (!mBActive) {
            mBTickEnabled = false;
        }

        UpdateTickRegistration();
    }
}

bool UActorComponent::CanChangeOuter(const UObject* NewOuter) const {
    return NewOuter == mOwner;
}

void UActorComponent::OnIdentityChanged() {
    UWorld* World{mOwner != nullptr ? mOwner->GetWorld() : nullptr};

    if (World != nullptr && !mOwner->IsBeingDestroyed()) {
        World->MarkStructureDirty();
    }
}
