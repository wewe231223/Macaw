#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/ULightComponent.h"

class ULocalLightComponent : public ULightComponent {
public:
    ReflectTypes(ULocalLightComponent, ULightComponent);

public:
    ULocalLightComponent();
    ~ULocalLightComponent() override = default;

public:
    ReflectClass(ULocalLightComponent, EClassFlags::Abstract);
    ReflectFunction(GetAttenuationRadius);

    float GetAttenuationRadius() const;
    ReflectFunction(SetAttenuationRadius, FName{"InAttenuationRadius"});

    void SetAttenuationRadius(float InAttenuationRadius);

    void BuildLightShaderParameters(FLightShaderParameters& OutParameters) const override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    ReflectAccessor(mAttenuationRadius, GetAttenuationRadius, SetAttenuationRadius, "AttenuationRadius", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mAttenuationRadius), ReflectEditor("Local Light", "Attenuation Radius"), ReflectRange(0.0f, std::numeric_limits<float>::max()), ReflectSpeed(1.0f));

    float mAttenuationRadius{1000.0f};

    ReflectMembers();
};
