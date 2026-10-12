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

bool UCollisionComponent::Raycast(const FRay& Ray, float& OutDistance) const {
    if (!IsRegistered() || IsBeingDestroyed() || !mBCollisionEnabled || !RaycastBounds(Ray, OutDistance)) {
        return false;
    }

    UMeshComponent* Mesh{GetMeshComponent()};

    return Mesh == nullptr || Mesh->RaycastMesh(Ray, OutDistance);
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
