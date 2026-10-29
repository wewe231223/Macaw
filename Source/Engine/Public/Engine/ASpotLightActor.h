#pragma once

#include "Engine/AActor.h"

class USpotLightComponent;

class ASpotLightActor final : public AActor {
public:
    ReflectTypes(ASpotLightActor, AActor);

public:
    ASpotLightActor();
    ~ASpotLightActor() override;

public:
    ReflectClass(ASpotLightActor);

    bool InitializeDefaultComponents() override;
    USpotLightComponent* GetLightComponent() const;

private:
    ReflectMembers();
};
