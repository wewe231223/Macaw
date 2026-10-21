#pragma once

#include "Engine/AActor.h"

class UStaticMeshComponent;

class AStaticMeshActor final : public AActor {
public:
    ReflectTypes(AStaticMeshActor, AActor);

public:
    AStaticMeshActor();
    ~AStaticMeshActor() override;

public:
    ReflectClass(AStaticMeshActor);

    bool InitializeDefaultComponents() override;
    UStaticMeshComponent* GetStaticMeshComponent() const;

private:
    ReflectMembers();
};
