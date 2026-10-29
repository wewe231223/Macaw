#pragma once

#include "Engine/AActor.h"

class UDirectionalLightComponent;

class ADirectionalLightActor final : public AActor {
public:
    ReflectTypes(ADirectionalLightActor, AActor);

public:
    ADirectionalLightActor();
    ~ADirectionalLightActor() override;

public:
    ReflectClass(ADirectionalLightActor);

    bool InitializeDefaultComponents() override;
    UDirectionalLightComponent* GetLightComponent() const;

private:
    ReflectMembers();
};
