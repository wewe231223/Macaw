#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/UWorld.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FBox.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FSceneInterface.h"
#include "Engine/PrimitiveMesh.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

UPrimitiveComponent::UPrimitiveComponent() = default;

UPrimitiveComponent::~UPrimitiveComponent() {
    UnregisterComponent();
}

ImplementReflection(UPrimitiveComponent, EClassFlags::Abstract)

void UPrimitiveComponent::RegisterComponent(UWorld& World) {
    if (GetWorld() == &World) {
        return;
    }

    UnregisterComponent();
    UActorComponent::RegisterComponent(World);
    MarkRenderStateDirty();
}

void UPrimitiveComponent::UnregisterComponent() {
    if (mRenderStateCreated && GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
        GetWorld()->GetScene()->RemovePrimitive(GetGuid());
    }

    mRenderStateCreated = false;
    mQueued = false;
    mRenderStateDirty = true;
    mRenderTransformDirty = false;

    UActorComponent::UnregisterComponent();
}

bool UPrimitiveComponent::SetTransform(const FTransform& Transform) {
    if (!USceneComponent::SetTransform(Transform)) {
        return false;
    }

    MarkRenderTransformDirty();

    return true;
}

void UPrimitiveComponent::MarkRenderStateDirty() {
    mRenderStateDirty = true;
    QueueUpdate();
}

void UPrimitiveComponent::MarkRenderTransformDirty() {
    mRenderTransformDirty = true;
    QueueUpdate();
}

void UPrimitiveComponent::QueueUpdate() {
    if (!mQueued && GetWorld() != nullptr) {
        mQueued = true;
        GetWorld()->EnqueueRenderUpdate(GetHandle());
    }
}

void UPrimitiveComponent::SendRenderUpdates() {
    mQueued = false;

    if (GetWorld() == nullptr || GetWorld()->GetScene() == nullptr) {
        return;
    }

    FSceneInterface& Scene{*GetWorld()->GetScene()};

    if (mRenderStateDirty || !mRenderStateCreated) {
        Scene.AddPrimitive(CreateSceneProxy());
        mRenderStateCreated = true;
    } else if (mRenderTransformDirty) {
        const FPrimitiveSceneProxy Proxy{CreateSceneProxy()};

        Scene.UpdatePrimitiveTransform(Proxy.mId, Proxy.mWorld, Proxy.mBounds);
    }

    mRenderStateDirty = false;
    mRenderTransformDirty = false;
}

FPrimitiveSceneProxy UPrimitiveComponent::CreateSceneProxy() const {
    const FMatrix World{GetWorldMatrix()};
    return {GetGuid(), GetPrimitiveType(), World, GetPrimitiveMesh(GetPrimitiveType()).mBounds.Transformed(World)};
}

FBox UPrimitiveComponent::GetWorldBounds() const {
    return GetPrimitiveMesh(GetPrimitiveType()).mBounds.Transformed(GetWorldMatrix());
}
