#include "pch.h"
#include "World/Component/UPrimitiveComponent.h"
#include "World/AActor.h"
#include "World/Subsystem/UPickingSubsystem.h"
#include "World/UWorld.h"

bool UPrimitiveComponent::IsVisible() const {
    return mBVisible;
}

void UPrimitiveComponent::SetVisible(bool BInVisible) {
    if (mBVisible == BInVisible) {
        return;
    }

    mBVisible = BInVisible;
    OnRenderStateChanged();
}

void UPrimitiveComponent::OnRenderStateChanged() {
    Super::OnRenderStateChanged();

    UWorld* World{GetBelongingWorld()};

    if (World != nullptr) {
        World->GetPickingSubsystem().UpdateComponent(this);
    }
}

void UPrimitiveComponent::OnRegister() {
    Super::OnRegister();

    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetPickingSubsystem().RegisterComponent(this);
        Owner->GetWorld()->GetRenderSubsystem().RegisterComponent(this);
    }
}

void UPrimitiveComponent::OnUnregister() {
    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetPickingSubsystem().UnregisterComponent(this);
        Owner->GetWorld()->GetRenderSubsystem().UnregisterComponent(this);
    }

    Super::OnUnregister();
}

std::unique_ptr<FPrimitiveSceneProxy> UPrimitiveComponent::CreateSceneProxy() const {
    return nullptr;
}

void UPrimitiveComponent::CreateRenderState() {
    UWorld* World{GetBelongingWorld()};

    if (World == nullptr || !IsRegistered() || IsRenderStateCreated() || !ShouldCreateRenderState()) {
        return;
    }

    std::unique_ptr<FPrimitiveSceneProxy> Proxy{CreateSceneProxy()};

    if (Proxy == nullptr) {
        return;
    }

    World->GetRenderSubsystem().AddPrimitive(std::move(Proxy));
    UActorComponent::CreateRenderState();
}

void UPrimitiveComponent::DestroyRenderState() {
    UWorld* World{GetBelongingWorld()};

    if (World != nullptr && IsRenderStateCreated()) {
        World->GetRenderSubsystem().RemovePrimitive(GetHandle());
    }

    UActorComponent::DestroyRenderState();
}

void UPrimitiveComponent::SendRenderTransform() {
    UWorld* World{GetBelongingWorld()};

    if (World != nullptr && IsRenderStateCreated()) {
        World->GetRenderSubsystem().UpdatePrimitiveTransform(GetHandle(), GetRenderTransform());
    }
}

FPrimitiveTransform UPrimitiveComponent::GetRenderTransform() const {
    return FPrimitiveTransform{GetComponentToWorld(), GetWorldSphere(), GetWorldOBB(), GetWorldAABB()};
}

void UPrimitiveComponent::SendRenderDynamicData() {
    UWorld* World{GetBelongingWorld()};

    if (World != nullptr && IsRenderStateCreated()) {
        World->GetRenderSubsystem().UpdatePrimitiveUV(GetHandle(), mUVTransforms);
    }
}

bool UPrimitiveComponent::SetUVTransform(FName MaterialSlot, const FUVTransform& Transform, FObjectHandle SourceHandle) {
    if (!std::isfinite(Transform.mScale.mX) || !std::isfinite(Transform.mScale.mY) || !std::isfinite(Transform.mOffset.mX) || !std::isfinite(Transform.mOffset.mY)) {
        return false;
    }

    for (FPrimitiveUVTransform& Entry : mUVTransforms) {
        if (Entry.mMaterialSlot != MaterialSlot) {
            continue;
        }

        if (Entry.mSourceHandle.IsValid() && Entry.mSourceHandle != SourceHandle && UObjectSystem::Resolve(Entry.mSourceHandle) != nullptr) {
            return false;
        }

        if (Entry.mTransform.mScale.mX == Transform.mScale.mX && Entry.mTransform.mScale.mY == Transform.mScale.mY && Entry.mTransform.mOffset.mX == Transform.mOffset.mX && Entry.mTransform.mOffset.mY == Transform.mOffset.mY && Entry.mSourceHandle == SourceHandle) {
            return true;
        }

        Entry.mTransform = Transform;
        Entry.mSourceHandle = SourceHandle;
        MarkRenderDynamicDataDirty();

        return true;
    }

    mUVTransforms.push_back(FPrimitiveUVTransform{MaterialSlot, Transform, SourceHandle});
    MarkRenderDynamicDataDirty();

    return true;
}

void UPrimitiveComponent::ClearUVTransform(FName MaterialSlot, FObjectHandle SourceHandle) {
    const std::size_t PreviousCount{mUVTransforms.size()};

    std::erase_if(mUVTransforms, [MaterialSlot, SourceHandle](const FPrimitiveUVTransform& Entry) {
        return Entry.mMaterialSlot == MaterialSlot && Entry.mSourceHandle == SourceHandle;
    });

    if (PreviousCount != mUVTransforms.size()) {
        MarkRenderDynamicDataDirty();
    }
}

const TArray<FPrimitiveUVTransform>& UPrimitiveComponent::GetUVTransforms() const {
    return mUVTransforms;
}

void UPrimitiveComponent::UpdateBounds() {
    const DirectX::XMMATRIX WorldMatrix{GetComponentToWorld().ToSimpleMath()};

    mLocalSphere.Transform(mWorldSphere, WorldMatrix);
    mPickingBox.Transform(mWorldOBB, WorldMatrix);
    mLocalAABB.Transform(mWorldAABB, WorldMatrix);

    mWorldBoundsTransformRevision = GetTransformRevision();
}

void UPrimitiveComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    if (Archive.IsLoading()) {
        OnRenderStateChanged();
    }
}

void UPrimitiveComponent::SetPickingBox(const DirectX::BoundingOrientedBox& Box) {
    mPickingBox = Box;

    BuildBoundsFromOBB();
}

const DirectX::BoundingOrientedBox& UPrimitiveComponent::GetPickingBox() const {
    return mPickingBox;
}

void UPrimitiveComponent::BuildBoundsFromOBB() {
    DirectX::BoundingSphere::CreateFromBoundingBox(mLocalSphere, mPickingBox);
    DirectX::BoundingBox::CreateFromSphere(mLocalAABB, mLocalSphere);

    UpdateBounds();
    OnTransformUpdate();
}

const DirectX::BoundingSphere& UPrimitiveComponent::GetBoundingSphere() const {
    return mLocalSphere;
}

const DirectX::BoundingBox& UPrimitiveComponent::GetWorldAABB() const {
    const Uint64 TransformRevision{GetTransformRevision()};

    if (mWorldBoundsTransformRevision != TransformRevision) {
        const_cast<UPrimitiveComponent*>(this)->UpdateBounds();
    }

    return mWorldAABB;
}

const DirectX::BoundingOrientedBox& UPrimitiveComponent::GetWorldOBB() const {
    const Uint64 TransformRevision{GetTransformRevision()};

    if (mWorldBoundsTransformRevision != TransformRevision) {
        const_cast<UPrimitiveComponent*>(this)->UpdateBounds();
    }

    return mWorldOBB;
}

const DirectX::BoundingSphere& UPrimitiveComponent::GetWorldSphere() const {
    const Uint64 TransformRevision{GetTransformRevision()};

    if (mWorldBoundsTransformRevision != TransformRevision) {
        const_cast<UPrimitiveComponent*>(this)->UpdateBounds();
    }

    return mWorldSphere;
}

void UPrimitiveComponent::OnTransformUpdate() {
    MarkRenderTransformDirty();

    UWorld* World{GetBelongingWorld()};

    if (World != nullptr) {
        World->GetPickingSubsystem().UpdateComponent(this);
    }
}

UPrimitiveComponent::UPrimitiveComponent() {
    InitializeClass(StaticClass());
}
