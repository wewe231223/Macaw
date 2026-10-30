#pragma once
#include "World/AActor.h"
#include "World/Component/UCameraComponent.h"

class ACameraActor : public AActor {
public:
    ReflectTypes(ACameraActor, AActor);

public:
    ACameraActor();
    ~ACameraActor() override = default;

public:
    ReflectClass(ACameraActor);

    UCameraComponent* GetCameraComponent() const;

private:
    UCameraComponent* mCameraComponent{};

    ReflectMembers();
};
