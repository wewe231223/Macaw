#pragma once
#include "World/Component/UPointLightComponent.h"
#include "World/Component/USpotLightComponent.generated.h"

UCLASS()
class USpotLightComponent : public UPointLightComponent {
public:
    GENERATED_TYPES(USpotLightComponent)

public:
    USpotLightComponent();
    ~USpotLightComponent() override = default;

public:
    GENERATED_BODY(USpotLightComponent)

    UFUNCTION()
    ELightType GetLightType() const override;

    UFUNCTION()
    float GetInnerConeAngle() const;
    UFUNCTION()
    float GetOuterConeAngle() const;
    UFUNCTION()
    void SetInnerConeAngle(float InInnerConeAngle);
    UFUNCTION()
    void SetOuterConeAngle(float InOuterConeAngle);

    void BuildLightShaderParameters(FLightShaderParameters& OutParameters) const override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    UPROPERTY(Serialize, Name = "InnerConeAngle", Getter = GetInnerConeAngle, Category = "Spot Light", DisplayName = "Inner Cone Angle", Edit, Setter = SetInnerConeAngle, Min = 0.0f, Max = 89.9f, MaxGetter = GetOuterConeAngle)
    float mInnerConeAngle{30.0f};
    UPROPERTY(Serialize, Name = "OuterConeAngle", Getter = GetOuterConeAngle, Category = "Spot Light", DisplayName = "Outer Cone Angle", Edit, Setter = SetOuterConeAngle, Min = 0.0f, Max = 89.9f, MinGetter = GetInnerConeAngle)
    float mOuterConeAngle{45.0f};
};
