#pragma once

#include "Engine/AActor.h"

class UCameraComponent;

class ACameraActor : public AActor {
public:
    ReflectTypes(ACameraActor, AActor);

public:
    ReflectClass(ACameraActor);
    bool InitializeDefaultComponents() override;
    UCameraComponent* GetCameraComponent() const;
    void Input(const FViewportInput& Input, float DeltaTime) override;

private:
    ReflectMembers();
};
