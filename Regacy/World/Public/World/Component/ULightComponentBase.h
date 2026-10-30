#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/USceneComponent.h"

class ULightComponentBase : public USceneComponent {
public:
    ReflectTypes(ULightComponentBase, USceneComponent);

public:
    ULightComponentBase();
    ~ULightComponentBase() override = default;

public:
    ReflectClass(ULightComponentBase, EClassFlags::Abstract);
    ReflectFunction(GetLightColor);

    const FVector3& GetLightColor() const;
    ReflectFunction(SetLightColor, FName{"InLightColor"});

    void SetLightColor(const FVector3& InLightColor);
    ReflectFunction(GetIntensity);

    float GetIntensity() const;
    ReflectFunction(SetIntensity, FName{"InIntensity"});

    void SetIntensity(float InIntensity);
    ReflectFunction(IsVisible);

    bool IsVisible() const;
    ReflectFunction(SetVisible, FName{"BInVisible"});

    void SetVisible(bool BInVisible);

protected:
    void Serialize(FArchive& Archive) override;

private:
    ReflectAccessor(mLightColor, GetLightColor, SetLightColor, "LightColor", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Color, ReflectSerializer(mLightColor), ReflectEditor("Light", "Color"));

    FVector3 mLightColor{1.0f, 1.0f, 1.0f};
    ReflectAccessor(mIntensity, GetIntensity, SetIntensity, "Intensity", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mIntensity), ReflectEditor("Light", "Intensity"), ReflectRange(0.0f, std::numeric_limits<float>::max()));

    float mIntensity{1.0f};
    ReflectAccessor(mBVisible, IsVisible, SetVisible, "bVisible", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mBVisible), ReflectEditor("Light", "Visible"));

    bool mBVisible{true};

    ReflectMembers();
};
