#pragma once

#include "Engine/Component/ULightComponent.h"

class UDirectionalLightComponent final : public ULightComponent {
public:
    ReflectTypes(UDirectionalLightComponent, ULightComponent);

public:
    UDirectionalLightComponent();
    ~UDirectionalLightComponent() override;

public:
    ReflectClass(UDirectionalLightComponent);

    FLightSceneProxy CreateLightProxy() const override;

private:
    ReflectMembers();
};
