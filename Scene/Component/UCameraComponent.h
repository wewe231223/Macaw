#pragma once

#include "USceneComponent.h"

class UCameraComponent : public USceneComponent
{
public:
    UCameraComponent() = default;
    ~UCameraComponent() override = default;

    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FMatrix GetViewProjectionMatrix() const;

    float GetFOV() const;
    float GetAspectRatio() const;
    float GetNearPlane() const;
    float GetFarPlane() const;

    void SetFOV(float InFOV);
    void SetAspectRatio(float InAspectRatio);
    void SetNearPlane(float InNearPlane);
    void SetFarPlane(float InFarPlane);

private:
    float FOV = 1.0472f;       // 약 60도, 라디안
    float AspectRatio = 16.0f / 9.0f;
    float NearPlane = 0.1f;
    float FarPlane = 1000.0f;
};