#pragma once

#include "Engine/Component/ULightComponent.h"

class UAmbientLightComponent final : public ULightComponent {
public:
    ReflectTypes(UAmbientLightComponent, ULightComponent);

public:
    UAmbientLightComponent();
    ~UAmbientLightComponent() override;

public:
    ReflectClass(UAmbientLightComponent);

    FLightSceneProxy CreateLightProxy() const override;

private:
    ReflectMembers();
};
