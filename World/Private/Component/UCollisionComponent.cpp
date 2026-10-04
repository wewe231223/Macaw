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
    UPrimitiveComponent::OnRegister();
    GetBelongingWorld()->GetCollisionSubsystem().RegisterComponent(this);
}

void UCollisionComponent::OnUnregister() {
    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetCollisionSubsystem().UnregisterComponent(this);
    }

    UPrimitiveComponent::OnUnregister();
}

bool UCollisionComponent::Raycast(const FRay& Ray, float& OutDistance) const {
    if (!IsRegistered() || IsBeingDestroyed() || !mBCollisionEnabled || !RaycastBounds(Ray, OutDistance)) {
        return false;
    }

    UMeshComponent* Mesh{GetMeshComponent()};

    return Mesh == nullptr || Mesh->RaycastMesh(Ray, OutDistance);
}

void UCollisionComponent::MakeRender(FActorProbe& Probe) const {
}

void UCollisionComponent::Serialize(FArchive& Archive) {
    UPrimitiveComponent::Serialize(Archive);

    Archive.Serialize("bCollisionEnabled", mBCollisionEnabled);
}

class UMeshComponent* UCollisionComponent::GetMeshComponent() const {
    return nullptr;
}
