#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "CoreUObject/Reflection/UClass.h"
#include "World/Component/USubUVComponent.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "Asset/UMaterial.h"

#include <cmath>
#include <limits>

USubUVComponent::USubUVComponent() {
    InitializeClass(StaticClass());
    SetCanEverTick(true);
    SetTickInEditor(true);
}

bool USubUVComponent::SetTargetComponent(UPrimitiveComponent* Target) {
    if (Target != nullptr && (Target->IsBeingDestroyed() || Target->GetOwner() != GetOwner())) {
        return false;
    }

    if (GetTargetComponent() != Target) {
        ClearTargetTransform();
        mTargetComponent.Set(Target);
        mPendingTargetGuid = {};
    }

    ApplyUVTransform();
    UpdateTickEnabled();
    return true;
}

UPrimitiveComponent* USubUVComponent::GetTargetComponent() const {
    return mTargetComponent.Get();
}

void USubUVComponent::SetTargetMaterialSlot(FName MaterialSlot) {
    if (mTargetMaterialSlot != MaterialSlot) {
        ClearTargetTransform();
        mTargetMaterialSlot = MaterialSlot;
    }

    ApplyUVTransform();
}

FName USubUVComponent::GetTargetMaterialSlot() const {
    return mTargetMaterialSlot;
}

bool USubUVComponent::HasTargetConflict() const {
    const UPrimitiveComponent* Target{GetTargetComponent()};

    if (Target != nullptr) {
        for (const FPrimitiveUVTransform& Entry : Target->GetUVTransforms()) {
            if (Entry.mMaterialSlot == mTargetMaterialSlot && Entry.mSourceHandle.IsValid() && Entry.mSourceHandle != GetHandle() && UObjectSystem::Resolve(Entry.mSourceHandle) != nullptr) {
                return true;
            }
        }
    }

    return false;
}

void USubUVComponent::SetUVTransform(const FUVTransform& Transform) {
    if (!std::isfinite(Transform.mScale.mX) || !std::isfinite(Transform.mScale.mY) || !std::isfinite(Transform.mOffset.mX) || !std::isfinite(Transform.mOffset.mY)) {
        return;
    }

    mUVTransform = Transform;
    ApplyUVTransform();
}

const FUVTransform& USubUVComponent::GetUVTransform() const {
    return mUVTransform;
}

void USubUVComponent::UpdateUVTransform() {
    SetUVTransform(CalculateUVTransform());
}

void USubUVComponent::Play() {
    if (mElapsedTime >= GetDuration()) {
        mElapsedTime = 0.0;
    }

    mPlaying = true;
    UpdateUVTransform();
    UpdateTickEnabled();
}

void USubUVComponent::Pause() {
    mPlaying = false;
    UpdateTickEnabled();
}

void USubUVComponent::Stop() {
    mPlaying = false;
    ResetPlaybackState();
    SetElapsedTime(0.0);
}

void USubUVComponent::Restart() {
    mPlaying = true;
    ResetPlaybackState();
    SetElapsedTime(0.0);
}

bool USubUVComponent::IsPlaying() const {
    return mPlaying;
}

void USubUVComponent::SetLooping(bool Looping) {
    mLooping = Looping;
    SetElapsedTime(mElapsedTime);
}

bool USubUVComponent::IsLooping() const {
    return mLooping;
}

void USubUVComponent::SetElapsedTime(double ElapsedTime) {
    if (!std::isfinite(ElapsedTime) || ElapsedTime < 0.0) {
        return;
    }

    const double Duration{GetDuration()};

    mElapsedTime = std::isfinite(Duration) && Duration > 0.0 ? (mLooping ? std::fmod(ElapsedTime, Duration) : (std::min)(ElapsedTime, Duration)) : ElapsedTime;
    UpdateUVTransform();
    UpdateTickEnabled();
}

double USubUVComponent::GetElapsedTime() const {
    return mElapsedTime;
}

void USubUVComponent::OnRegister() {
    Super::OnRegister();
    UpdateUVTransform();
    UpdateTickEnabled();
}

void USubUVComponent::OnUnregister() {
    ClearTargetTransform();
    Super::OnUnregister();
}

void USubUVComponent::Activate() {
    Super::Activate();
    UpdateTickEnabled();
}

void USubUVComponent::TickComponent(float DeltaTime) {
    Super::TickComponent(DeltaTime);

    if (GetTargetComponent() == nullptr) {
        UpdateTickEnabled();
        return;
    }

    if (!IsActive() || !mPlaying || !CanAdvance() || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) {
        return;
    }

    const double Duration{GetDuration()};
    const double NextTime{mElapsedTime + DeltaTime};

    if (!mLooping && std::isfinite(Duration) && NextTime >= Duration) {
        mPlaying = false;
    }

    SetElapsedTime(NextTime);
}

bool USubUVComponent::ResolveLoadedReferences() {
    if (!UActorComponent::ResolveLoadedReferences()) {
        return false;
    }

    if (mPendingTargetGuid.IsValid()) {
        UObject* Object{UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(mPendingTargetGuid))};

        if (Object == nullptr || !Object->GetClass()->IsChildOf(UPrimitiveComponent::StaticClass()) || !SetTargetComponent(static_cast<UPrimitiveComponent*>(Object))) {
            return false;
        }
    }

    return true;
}

void USubUVComponent::Serialize(FArchive& Archive) {
    if (Archive.IsLoading()) {
        ClearTargetTransform();
    }

    Super::Serialize(Archive);

    if (Archive.IsLoading() && Archive.SupportsNamedFields() && !Archive.HasField("TargetMaterialSlot") && Archive.HasField("TargetMaterialGroup")) {
        Archive.Serialize("TargetMaterialGroup", mTargetMaterialSlot);
    }

    if (Archive.IsLoading()) {
        mElapsedTime = 0.0;
        UpdateTickEnabled();
    }
}

void USubUVComponent::UpdateTickEnabled() {
    SetTickEnabled(IsActive() && mPlaying && CanAdvance() && GetTargetComponent() != nullptr);
}

FUVTransform USubUVComponent::CalculateUVTransform() const {
    return mUVTransform;
}

bool USubUVComponent::CanAdvance() const {
    return false;
}

double USubUVComponent::GetDuration() const {
    return std::numeric_limits<double>::infinity();
}

void USubUVComponent::ApplyUVTransform() {
    if (UPrimitiveComponent* Target{GetTargetComponent()}; Target != nullptr && !Target->IsBeingDestroyed() && GetHandle().IsValid() && IsRegistered()) {
        Target->SetUVTransform(mTargetMaterialSlot, mUVTransform, GetHandle());
    }
}

void USubUVComponent::ResetPlaybackState() {
}

void USubUVComponent::ClearTargetTransform() {
    if (UPrimitiveComponent* Target{GetTargetComponent()}) {
        Target->ClearUVTransform(mTargetMaterialSlot, GetHandle());
    }
}

bool USubUVComponent::CanSelectTarget(UPrimitiveComponent* Target) const {
    return Target == nullptr || (!Target->IsBeingDestroyed() && Target->GetOwner() == GetOwner() && (Target->IsA(UStaticMeshComponent::StaticClass()) || Target->IsA(UBillboardComponent::StaticClass())));
}

bool USubUVComponent::CanEditUVTransform() const {
    return GetClass() == StaticClass();
}

TArray<FName> USubUVComponent::GetTargetMaterialSlots() const {
    TArray<FName> Slots{};
    const UPrimitiveComponent* Target{GetTargetComponent()};
    const UMesh* Mesh{Target != nullptr && Target->IsA(UStaticMeshComponent::StaticClass()) ? static_cast<const UStaticMeshComponent*>(Target)->ResolveMesh() : nullptr};

    if (Mesh != nullptr) {
        for (const FMeshMaterialSlot& Slot : Mesh->GetMaterialSlots()) {
            Slots.push_back(Slot.mName);
        }
    }

    return Slots;
}

void USubUVComponent::SerializeTargetComponentProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    USubUVComponent& Object{*static_cast<USubUVComponent*>(Container)};

    SerializeObjectReference(Archive, Property.GetSerializationName(), Object.GetTargetComponent(), Object.mPendingTargetGuid);

    if (Archive.IsLoading()) {
        Object.mTargetComponent = {};
    }
}
