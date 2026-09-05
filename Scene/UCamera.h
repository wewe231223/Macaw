#pragma once

#include "USceneObject.h"

class UCamera : public USceneObject
{
public:
    UCamera();
    virtual ~UCamera() override = default;

    float GetFOV() const;
    float GetAspectRatio() const;
    float GetNearPlane() const;
    float GetFarPlane() const;

    void SetFOV(float InFOV);
    void SetAspectRatio(float InAspectRatio);
    void SetNearPlane(float InNearPlane);
    void SetFarPlane(float InFarPlane);

    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FMatrix GetViewProjectionMatrix() const;

private:
    float FOV;
    float AspectRatio;
    float NearPlane;
    float FarPlane;
};