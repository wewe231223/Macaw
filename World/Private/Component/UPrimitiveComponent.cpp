#include "pch.h"
#include "World/Component/UPrimitiveComponent.h"
#include "World/AActor.h"
#include "World/Subsystem/UPickingSubsystem.h"
#include "World/UWorld.h"

void UPrimitiveComponent::MakeRender(FActorProbe& OutProbe) const {
}

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
    USceneComponent::OnRenderStateChanged();

    UWorld* World{GetBelongingWorld()};

    if (World != nullptr) {
        World->GetPickingSubsystem().UpdateComponent(this);
    }
}

void UPrimitiveComponent::OnRegister() {
    USceneComponent::OnRegister();

    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetPickingSubsystem().RegisterComponent(this);
    }
}

void UPrimitiveComponent::OnUnregister() {
    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetPickingSubsystem().UnregisterComponent(this);
    }

    USceneComponent::OnUnregister();
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

void UPrimitiveComponent::UpdateBounds() {
    const DirectX::XMMATRIX WorldMatrix{GetComponentToWorld().ToSimpleMath()};

    mLocalSphere.Transform(mWorldSphere, WorldMatrix);
    mPickingBox.Transform(mWorldOBB, WorldMatrix);
    mLocalAABB.Transform(mWorldAABB, WorldMatrix);

    mWorldBoundsTransformRevision = GetTransformRevision();
    mWorldBoundsDirty = false;
}

void UPrimitiveComponent::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);

    Archive.Serialize("bVisible", mBVisible);

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

    OnTransformUpdate();
}

const DirectX::BoundingSphere& UPrimitiveComponent::GetBoundingSphere() const {
    return mLocalSphere;
}

const DirectX::BoundingBox& UPrimitiveComponent::GetWorldAABB() const {
    const Uint64 TransformRevision{GetTransformRevision()};

    if (mWorldBoundsDirty || mWorldBoundsTransformRevision != TransformRevision) {
        const_cast<UPrimitiveComponent*>(this)->UpdateBounds();
    }

    return mWorldAABB;
}

const DirectX::BoundingOrientedBox& UPrimitiveComponent::GetWorldOBB() const {
    const Uint64 TransformRevision{GetTransformRevision()};

    if (mWorldBoundsDirty || mWorldBoundsTransformRevision != TransformRevision) {
        const_cast<UPrimitiveComponent*>(this)->UpdateBounds();
    }

    return mWorldOBB;
}

const DirectX::BoundingSphere& UPrimitiveComponent::GetWorldSphere() const {
    const Uint64 TransformRevision{GetTransformRevision()};

    if (mWorldBoundsDirty || mWorldBoundsTransformRevision != TransformRevision) {
        const_cast<UPrimitiveComponent*>(this)->UpdateBounds();
    }

    return mWorldSphere;
}

void UPrimitiveComponent::OnTransformUpdate() {
    mWorldBoundsDirty = true;
    MarkRenderTransformDirty();

    UWorld* World{GetBelongingWorld()};

    if (World != nullptr) {
        World->GetPickingSubsystem().UpdateComponent(this);
    }
}
