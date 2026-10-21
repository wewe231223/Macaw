#pragma once
#include "World/AActor.h"
#include "World/Component/UPointLightComponent.h"

class APointLightActor : public AActor {
public:
    ReflectTypes(APointLightActor, AActor);

public:
    APointLightActor();
    ~APointLightActor() override = default;

public:
    ReflectClass(APointLightActor);
    UPointLightComponent* GetLightComponent() const;

private:
    UPointLightComponent* mLightComponent{};

    ReflectMembers();
};
