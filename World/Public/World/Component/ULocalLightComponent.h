#pragma once
#include "World/Component/ULightComponent.h"
#include "World/Component/ULocalLightComponent.generated.h"

UCLASS(Abstract)
class ULocalLightComponent : public ULightComponent {
public:
    GENERATED_TYPES(ULocalLightComponent)

public:
    ULocalLightComponent();
    ~ULocalLightComponent() override = default;

public:
    GENERATED_BODY(ULocalLightComponent)

    UFUNCTION()
    float GetAttenuationRadius() const;
    UFUNCTION()
    void SetAttenuationRadius(float InAttenuationRadius);

    void BuildLightShaderParameters(FLightShaderParameters& OutParameters) const override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    UPROPERTY(Serialize, Name = "AttenuationRadius", Getter = GetAttenuationRadius, Category = "Local Light", DisplayName = "Attenuation Radius", Edit, Setter = SetAttenuationRadius, Min = 0.0f, Speed = 1.0f)
    float mAttenuationRadius{1000.0f};
};
