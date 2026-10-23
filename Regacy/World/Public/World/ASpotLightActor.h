#pragma once
#include "World/AActor.h"
#include "World/Component/USpotLightComponent.h"

class ASpotLightActor : public AActor {
public:
    ReflectTypes(ASpotLightActor, AActor);

public:
    ASpotLightActor();
    ~ASpotLightActor() override = default;

public:
    ReflectClass(ASpotLightActor);
    USpotLightComponent* GetLightComponent() const;

private:
    USpotLightComponent* mLightComponent{};

    ReflectMembers();
};
