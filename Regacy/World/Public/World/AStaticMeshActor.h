#pragma once
#include "World/AActor.h"
#include "World/Component/UStaticMeshComponent.h"

class AStaticMeshActor : public AActor {
public:
    ReflectTypes(AStaticMeshActor, AActor);

public:
    AStaticMeshActor();
    ~AStaticMeshActor() override = default;

public:
    ReflectClass(AStaticMeshActor);
    UStaticMeshComponent* GetStaticMeshComponent() const;

private:
    UStaticMeshComponent* mStaticMeshComponent{};

    ReflectMembers();
};
