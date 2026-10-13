#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/ULocalLightComponent.h"

class UPointLightComponent : public ULocalLightComponent {
public:
    ReflectTypes(UPointLightComponent, ULocalLightComponent);

public:
    UPointLightComponent();
    ~UPointLightComponent() override = default;

public:
    ReflectClass(UPointLightComponent);

    ELightType GetLightType() const override;
};
