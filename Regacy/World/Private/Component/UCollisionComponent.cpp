#include "pch.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Component/UMeshComponent.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include "World/Subsystem/UCollisionSubsystem.h"
#include "Asset/UMesh.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Core/Console/Console.h"

bool UCollisionComponent::IsCollisionEnabled() const {
    return mBCollisionEnabled;
}

void UCollisionComponent::SetCollisionEnabled(bool BEnabled) {
    mBCollisionEnabled = BEnabled;
}

void UCollisionComponent::OnRegister() {
    Super::OnRegister();
    GetBelongingWorld()->GetCollisionSubsystem().RegisterComponent(this);
}

void UCollisionComponent::OnUnregister() {
    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetCollisionSubsystem().UnregisterComponent(this);
    }

    Super::OnUnregister();
}

bool UCollisionComponent::Raycast(const FRay& Ray, FWorldRaycastHit& OutHit, float MaxDistance) const {
    OutHit = {};
    FRay WorldRay{};
    const FWorldRaycastQuery Query{MaxDistance};

    if (!IsRegistered() || IsBeingDestroyed() || !mBCollisionEnabled || !Query.PrepareRay(Ray, WorldRay)) {
        return false;
    }

    UMeshComponent* Mesh{GetMeshComponent()};

    if (!RaycastBounds(WorldRay, OutHit, Mesh != nullptr ? std::numeric_limits<float>::max() : MaxDistance) || (Mesh != nullptr && !Mesh->RaycastMesh(WorldRay, OutHit, MaxDistance))) {
        return false;
    }

    OutHit.mComponent.Set(const_cast<UCollisionComponent*>(this));
    return true;
}

void UCollisionComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

}

class UMeshComponent* UCollisionComponent::GetMeshComponent() const {
    return nullptr;
}

UCollisionComponent::UCollisionComponent() {
    InitializeClass(StaticClass());
}
