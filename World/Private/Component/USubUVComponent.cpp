#include "pch.h"
#include "World/Component/USubUVComponent.h"
#include "World/AActor.h"

#include <cmath>
#include <limits>

USubUVComponent::USubUVComponent() {
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

void USubUVComponent::SetTargetMaterialGroup(FName MaterialGroup) {
    if (mTargetMaterialGroup != MaterialGroup) {
        ClearTargetTransform();
        mTargetMaterialGroup = MaterialGroup;
    }

    ApplyUVTransform();
}

FName USubUVComponent::GetTargetMaterialGroup() const {
    return mTargetMaterialGroup;
}

bool USubUVComponent::HasTargetConflict() const {
    const UPrimitiveComponent* Target{GetTargetComponent()};

    if (Target != nullptr) {
        for (const FPrimitiveUVTransform& Entry : Target->GetUVTransforms()) {
            if (Entry.mMaterialGroup == mTargetMaterialGroup && Entry.mSourceHandle.IsValid() && Entry.mSourceHandle != GetHandle() && UObjectSystem::Resolve(Entry.mSourceHandle) != nullptr) {
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
    UActorComponent::OnRegister();
    UpdateUVTransform();
    UpdateTickEnabled();
}

void USubUVComponent::OnUnregister() {
    ClearTargetTransform();
    UActorComponent::OnUnregister();
}

void USubUVComponent::Activate() {
    UActorComponent::Activate();
    UpdateTickEnabled();
}

void USubUVComponent::TickComponent(float DeltaTime) {
    UActorComponent::TickComponent(DeltaTime);

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

        if (Object == nullptr || !Object->GetTypeInfo()->IsA(UPrimitiveComponent::StaticTypeInfo()) || !SetTargetComponent(static_cast<UPrimitiveComponent*>(Object))) {
            return false;
        }
    }

    return true;
}

void USubUVComponent::Serialize(FArchive& Archive) {
    UActorComponent::Serialize(Archive);

    FGuid TargetGuid{GetTargetComponent() != nullptr ? GetTargetComponent()->GetGuid() : FGuid{}};

    Archive.Serialize("GuidTargetComponent", TargetGuid);
    Archive.Serialize("TargetMaterialGroup", mTargetMaterialGroup);
    Archive.Serialize("UVScale", mUVTransform.mScale);
    Archive.Serialize("UVOffset", mUVTransform.mOffset);
    Archive.Serialize("bPlaying", mPlaying);
    Archive.Serialize("bLooping", mLooping);

    if (Archive.IsLoading()) {
        ClearTargetTransform();
        mTargetComponent.Reset();
        mPendingTargetGuid = TargetGuid;
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
        Target->SetUVTransform(mTargetMaterialGroup, mUVTransform, GetHandle());
    }
}

void USubUVComponent::ResetPlaybackState() {
}

void USubUVComponent::ClearTargetTransform() {
    if (UPrimitiveComponent* Target{GetTargetComponent()}) {
        Target->ClearUVTransform(mTargetMaterialGroup, GetHandle());
    }
}
