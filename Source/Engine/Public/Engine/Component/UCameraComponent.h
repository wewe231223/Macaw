#pragma once

#include "Engine/Component/USceneComponent.h"

struct FSceneView;
struct FVector2;

class UCameraComponent final : public USceneComponent {
public:
    ReflectTypes(UCameraComponent, USceneComponent);

public:
    ReflectClass(UCameraComponent);
    float GetFieldOfView() const;
    bool SetFieldOfView(float FieldOfView);
    bool GetCameraView(const FVector2& Size, FSceneView& View) const;

private:
    ReflectMembers();

private:
    float mFieldOfView{60.0f};
    ReflectAccessor(mFieldOfView, GetFieldOfView, SetFieldOfView, "FieldOfView", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
