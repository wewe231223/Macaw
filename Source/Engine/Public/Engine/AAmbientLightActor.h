#pragma once

#include "Engine/AActor.h"

class UAmbientLightComponent;

class AAmbientLightActor final : public AActor {
public:
    ReflectTypes(AAmbientLightActor, AActor);

public:
    AAmbientLightActor();
    ~AAmbientLightActor() override;

public:
    ReflectClass(AAmbientLightActor);

    bool InitializeDefaultComponents() override;
    UAmbientLightComponent* GetLightComponent() const;

private:
    ReflectMembers();
};
