#include "pch.h"
#include "World/AStaticMeshActor.h"

AStaticMeshActor::AStaticMeshActor() {
    InitializeClass(StaticClass());
    mStaticMeshComponent = CreateDefaultComponent<UStaticMeshComponent>(FName{"Mesh"});
    SetRootComponent(mStaticMeshComponent);
}

UStaticMeshComponent* AStaticMeshActor::GetStaticMeshComponent() const {
    return FindDefaultComponent(FName{"Mesh"}) == mStaticMeshComponent ? mStaticMeshComponent : nullptr;
}
