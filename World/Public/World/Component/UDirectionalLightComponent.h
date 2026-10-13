#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/ULightComponent.h"

class UDirectionalLightComponent : public ULightComponent {
public:
    ReflectTypes(UDirectionalLightComponent, ULightComponent);

public:
    UDirectionalLightComponent();
    ~UDirectionalLightComponent() override = default;

public:
    ReflectClass(UDirectionalLightComponent);

    ELightType GetLightType() const override;
};
