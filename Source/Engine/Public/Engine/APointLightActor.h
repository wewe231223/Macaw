#pragma once

#include "Engine/AActor.h"

class UPointLightComponent;

class APointLightActor final : public AActor {
public:
    ReflectTypes(APointLightActor, AActor);

public:
    APointLightActor();
    ~APointLightActor() override;

public:
    ReflectClass(APointLightActor);

    bool InitializeDefaultComponents() override;
    UPointLightComponent* GetLightComponent() const;

private:
    ReflectMembers();
};
