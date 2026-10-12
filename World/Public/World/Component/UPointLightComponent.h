#pragma once
#include "World/Component/ULocalLightComponent.h"
#include "World/Component/UPointLightComponent.generated.h"

UCLASS()
class UPointLightComponent : public ULocalLightComponent {
public:
    GENERATED_TYPES(UPointLightComponent)

public:
    UPointLightComponent();
    ~UPointLightComponent() override = default;

public:
    GENERATED_BODY(UPointLightComponent)

    ELightType GetLightType() const override;
};
