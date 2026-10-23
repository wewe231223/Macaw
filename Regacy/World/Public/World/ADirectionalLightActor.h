#pragma once
#include "World/AActor.h"
#include "World/Component/UDirectionalLightComponent.h"

class ADirectionalLightActor : public AActor {
public:
    ReflectTypes(ADirectionalLightActor, AActor);

public:
    ADirectionalLightActor();
    ~ADirectionalLightActor() override = default;

public:
    ReflectClass(ADirectionalLightActor);
    UDirectionalLightComponent* GetLightComponent() const;

private:
    UDirectionalLightComponent* mLightComponent{};

    ReflectMembers();
};
