#include "pch.h"
#include "World/Component/UActorComponent.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include "Core/Base/ErrorHandler.h"

FName UActorComponent::GetDefaultComponentName() const {
    return mDefaultComponentName;
}

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
    MarkRenderStateDirty();
}

bool UActorComponent::ShouldCreateRenderState() const {
    return false;
}

void UActorComponent::CreateRenderState() {
    if (!mState.IsSet(EStateFlag::Registered) || mState.IsSet(EStateFlag::RenderStateCreated) || !ShouldCreateRenderState()) {
        return;
    }

    mState.Set(EStateFlag::RenderStateCreated);
    mState.Clear(EStateFlag::RenderStateDirty);
    mState.Clear(EStateFlag::RenderTransformDirty);
    mState.Clear(EStateFlag::RenderDynamicDataDirty);
}

void UActorComponent::DestroyRenderState() {
    mState.Clear(EStateFlag::RenderStateCreated);
    mState.Clear(EStateFlag::RenderStateDirty);
    mState.Clear(EStateFlag::RenderTransformDirty);
    mState.Clear(EStateFlag::RenderDynamicDataDirty);
}

void UActorComponent::SendRenderTransform() {
}

void UActorComponent::SendRenderDynamicData() {
}

bool UActorComponent::IsRenderStateCreated() const {
    return mState.IsSet(EStateFlag::RenderStateCreated);
}

void UActorComponent::MarkRenderStateDirty() {
    if (!mState.IsSet(EStateFlag::Registered) || mState.IsSet(EStateFlag::Unregistering) || (!mState.IsSet(EStateFlag::RenderStateCreated) && !ShouldCreateRenderState())) {
        return;
    }

    mState.Set(EStateFlag::RenderStateDirty);
    mParentWorld->GetRenderSubsystem().MarkComponentDirty(this);
}

void UActorComponent::MarkRenderTransformDirty() {
    if (!mState.IsSet(EStateFlag::Registered) || mState.IsSet(EStateFlag::Unregistering) || !mState.IsSet(EStateFlag::RenderStateCreated)) {
        return;
    }

    mState.Set(EStateFlag::RenderTransformDirty);
    mParentWorld->GetRenderSubsystem().MarkComponentDirty(this);
}

void UActorComponent::MarkRenderDynamicDataDirty() {
    if (!mState.IsSet(EStateFlag::Registered) || mState.IsSet(EStateFlag::Unregistering)) {
        return;
    }

    if (!mState.IsSet(EStateFlag::RenderStateCreated)) {
        MarkRenderStateDirty();
        return;
    }

    mState.Set(EStateFlag::RenderDynamicDataDirty);
    mParentWorld->GetRenderSubsystem().MarkComponentDirty(this);
}

void UActorComponent::DoDeferredRenderUpdates() {
    if (!mState.IsSet(EStateFlag::Registered) || mState.IsSet(EStateFlag::Unregistering)) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mParentWorld};

    if (mState.IsSet(EStateFlag::RenderStateDirty)) {
        if (mState.IsSet(EStateFlag::RenderStateCreated)) {
            DestroyRenderState();
        }

        mState.Clear(EStateFlag::RenderStateDirty);
        mState.Clear(EStateFlag::RenderTransformDirty);
        mState.Clear(EStateFlag::RenderDynamicDataDirty);

        if (mState.IsSet(EStateFlag::Registered) && !mState.IsSet(EStateFlag::BeingDestroyed) && !mOwner->IsBeingDestroyed() && ShouldCreateRenderState()) {
            CreateRenderState();
        }
    } else {
        if (mState.IsSet(EStateFlag::RenderTransformDirty)) {
            mState.Clear(EStateFlag::RenderTransformDirty);
            SendRenderTransform();
        }

        if (mState.IsSet(EStateFlag::RenderDynamicDataDirty)) {
            mState.Clear(EStateFlag::RenderDynamicDataDirty);
            SendRenderDynamicData();
        }
    }
}

bool UActorComponent::IsActive() const {
    return mBActive;
}

void UActorComponent::SetActive(bool BInActive) {
    if (mBActive == BInActive || mState.IsSet(EStateFlag::BeingDestroyed)) {
        return;
    }

    if (BInActive) {
        Activate();
    } else {
        Deactivate();
    }
}

void UActorComponent::Activate() {
    if (mState.IsSet(EStateFlag::BeingDestroyed)) {
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
    return mPrimaryComponentTick.CanEverTick();
}

void UActorComponent::SetCanEverTick(bool CanEverTick) {
    mPrimaryComponentTick.SetCanEverTick(CanEverTick);
    UpdateTickRegistration();
}

bool UActorComponent::IsTickEnabled() const {
    return mPrimaryComponentTick.IsTickEnabled();
}

void UActorComponent::SetTickEnabled(bool TickEnabled) {
    mPrimaryComponentTick.SetTickEnabled(TickEnabled);
    UpdateTickRegistration();
}

bool UActorComponent::IsTickInEditor() const {
    return mPrimaryComponentTick.IsTickInEditor();
}

void UActorComponent::SetTickInEditor(bool TickInEditor) {
    mPrimaryComponentTick.SetTickInEditor(TickInEditor);
    UpdateTickRegistration();
}

bool UActorComponent::IsTickWhenPaused() const {
    return mPrimaryComponentTick.IsTickWhenPaused();
}

void UActorComponent::SetTickWhenPaused(bool TickWhenPaused) {
    mPrimaryComponentTick.SetTickWhenPaused(TickWhenPaused);
}

const FActorComponentTickFunction& UActorComponent::GetTickFunction() const {
    return mPrimaryComponentTick;
}

void UActorComponent::SetWantsInitializeComponent(bool WantsInitializeComponent) {
    mState.Set(EStateFlag::WantsInitialize, WantsInitializeComponent);

    if (WantsInitializeComponent && mOwner != nullptr && mOwner->GetWorld() != nullptr && mOwner->GetWorld()->IsDeferringLifecycleChanges()) {
        mOwner->GetWorld()->mPendingLifecycleRequests.push_back({UWorld::ELifecycleRequestType::AdvanceComponent, this});
    }
}

bool UActorComponent::IsBeingDestroyed() const {
    return mState.HasAny({EStateFlag::BeingDestroyed, EStateFlag::DestroyRequested});
}

void UActorComponent::UpdateTickRegistration() {
    if (mParentWorld == nullptr) {
        return;
    }

    if (mPrimaryComponentTick.CanTick(mParentWorld->GetDefaultTickType())) {
        mParentWorld->RegisterTickComponent(this);
    } else {
        mParentWorld->UnregisterTickComponent(this);
    }
}

bool UActorComponent::IsRegistered() const {
    return mState.IsSet(EStateFlag::Registered);
}

bool UActorComponent::IsInitialized() const {
    return mState.IsSet(EStateFlag::Initialized);
}

bool UActorComponent::HasBegunPlay() const {
    return mState.IsSet(EStateFlag::HasBegunPlay);
}

UWorld* UActorComponent::GetBelongingWorld() const {
    return mParentWorld;
}

void UActorComponent::RegisterComponent(UWorld* World) {
    if (mState.HasAny({EStateFlag::Registering, EStateFlag::Unregistering}) || IsBeingDestroyed() || (mOwner != nullptr && mOwner->IsBeingDestroyed())) {
        return;
    }

    ErrorHandler::Report(mOwner == nullptr || World == nullptr, "UActorComponent", "Component registration requires an owner and a world", ErrorHandler::EErrorLevel::Critical);
    ErrorHandler::Report(World != mOwner->GetWorld(), "UActorComponent", "Component world must match its owner", ErrorHandler::EErrorLevel::Critical);

    if (World->IsDeferringLifecycleChanges()) {
        World->mPendingLifecycleRequests.push_back({UWorld::ELifecycleRequestType::RegisterComponent, this});
        return;
    }

    if (mState.IsSet(EStateFlag::Registered)) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{World};

    mParentWorld = World;
    mState.Set(EStateFlag::Registered);
    mState.Set(EStateFlag::Registering);
    OnRegister();

    if (mState.IsSet(EStateFlag::Registered) && !mState.IsSet(EStateFlag::RenderStateCreated) && !mState.IsSet(EStateFlag::BeingDestroyed) && !mOwner->IsBeingDestroyed() && ShouldCreateRenderState()) {
        CreateRenderState();
    }

    mState.Clear(EStateFlag::Registering);

    mOwner->OnComponentRegistered(*this);
    UpdateTickRegistration();
}

void UActorComponent::UnregisterComponent() {
    if (mState.IsSet(EStateFlag::Unregistering)) {
        return;
    }

    UWorld* World{mOwner != nullptr ? mOwner->GetWorld() : mParentWorld};

    if (World != nullptr && World->IsDeferringLifecycleChanges()) {
        World->mPendingLifecycleRequests.push_back({UWorld::ELifecycleRequestType::UnregisterComponent, this});
        return;
    }

    if (!mState.IsSet(EStateFlag::Registered)) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mParentWorld};

    mState.Set(EStateFlag::Unregistering);

    if (mState.IsSet(EStateFlag::RenderStateCreated)) {
        DestroyRenderState();
    }

    mState.Clear(EStateFlag::Registered);
    UpdateTickRegistration();
    OnUnregister();
    mParentWorld = nullptr;
    mState.Clear(EStateFlag::Unregistering);
}

void UActorComponent::DispatchInitializeComponent() {
    if (!mState.IsSet(EStateFlag::WantsInitialize) || mState.IsSet(EStateFlag::Initialized) || !mState.IsSet(EStateFlag::Registered) || IsBeingDestroyed()) {
        return;
    }

    mState.Set(EStateFlag::Initialized);
    InitializeComponent();
}

void UActorComponent::DispatchUninitializeComponent() {
    if (!mState.IsSet(EStateFlag::Initialized)) {
        return;
    }

    mState.Clear(EStateFlag::Initialized);
    UninitializeComponent();
}

void UActorComponent::DispatchBeginPlay() {
    if (!mState.IsSet(EStateFlag::Registered) || mState.IsSet(EStateFlag::HasBegunPlay) || IsBeingDestroyed()) {
        return;
    }

    mState.Set(EStateFlag::HasBegunPlay);
    BeginPlay();
    UpdateTickRegistration();
}

void UActorComponent::DispatchEndPlay(EEndPlayReason Reason) {
    if (!mState.IsSet(EStateFlag::HasBegunPlay)) {
        return;
    }

    mState.Clear(EStateFlag::HasBegunPlay);
    UpdateTickRegistration();
    EndPlay(Reason);
}

bool UActorComponent::DeferComponentDestruction(bool BPromoteChildren) {
    if (IsBeingDestroyed()) {
        return true;
    }

    UWorld* World{mOwner != nullptr ? mOwner->GetWorld() : nullptr};

    if (World != nullptr && World->IsDeferringLifecycleChanges()) {
        mState.Set(EStateFlag::DestroyRequested);
        World->mPendingLifecycleRequests.push_back({UWorld::ELifecycleRequestType::DestroyComponent, this, {}, {}, EEndPlayReason::Destroyed, BPromoteChildren});
        return true;
    }

    return false;
}

void UActorComponent::ApplyDestruction(bool BPromoteChildren) {
    mState.Clear(EStateFlag::DestroyRequested);
    DestroyComponent(BPromoteChildren);
}

void UActorComponent::DestroyComponent(bool BPromoteChildren) {
    if (DeferComponentDestruction(BPromoteChildren)) {
        return;
    }

    const UWorld::FActorDispatchScope Dispatch{mOwner != nullptr ? mOwner->GetWorld() : nullptr};

    mState.Set(EStateFlag::BeingDestroyed);
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
    Super::Serialize(Archive);

    if (Archive.SupportsNamedFields() && (Archive.IsSaving() || Archive.HasField("DefaultComponentName"))) {
        FName DefaultComponentName{mDefaultComponentName};

        Archive.Serialize("DefaultComponentName", DefaultComponentName);

        if (Archive.IsLoading() && DefaultComponentName != mDefaultComponentName) {
            Archive.SetError();
        }
    }

    if (Archive.IsLoading()) {
        if (!mBActive) {
            mPrimaryComponentTick.SetTickEnabled(false);
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

UActorComponent::UActorComponent() {
    InitializeClass(StaticClass());
}
