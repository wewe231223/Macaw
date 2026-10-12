#pragma once
#include "World/Component/ULightComponent.h"
#include "World/Component/UDirectionalLightComponent.generated.h"

UCLASS()
class UDirectionalLightComponent : public ULightComponent {
public:
    GENERATED_TYPES(UDirectionalLightComponent)

public:
    UDirectionalLightComponent();
    ~UDirectionalLightComponent() override = default;

public:
    GENERATED_BODY(UDirectionalLightComponent)

    ELightType GetLightType() const override;
};
