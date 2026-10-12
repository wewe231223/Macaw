#pragma once
#include "World/Component/USceneComponent.h"
#include "World/Component/ULightComponentBase.generated.h"

UCLASS(Abstract)
class ULightComponentBase : public USceneComponent {
public:
    GENERATED_TYPES(ULightComponentBase)

public:
    ULightComponentBase();
    ~ULightComponentBase() override = default;

public:
    GENERATED_BODY(ULightComponentBase)
    UFUNCTION()
    const FVector3& GetLightColor() const;
    UFUNCTION()
    void SetLightColor(const FVector3& InLightColor);

    UFUNCTION()
    float GetIntensity() const;
    UFUNCTION()
    void SetIntensity(float InIntensity);

    UFUNCTION()
    bool IsVisible() const;
    UFUNCTION()
    void SetVisible(bool BInVisible);

protected:
    void Serialize(FArchive& Archive) override;

private:
    UPROPERTY(Serialize, Name = "LightColor", Getter = GetLightColor, Category = "Light", DisplayName = "Color", Edit, Setter = SetLightColor, Color)
    FVector3 mLightColor{1.0f, 1.0f, 1.0f};
    UPROPERTY(Serialize, Name = "Intensity", Getter = GetIntensity, Category = "Light", DisplayName = "Intensity", Edit, Setter = SetIntensity, Min = 0.0f)
    float mIntensity{1.0f};
    UPROPERTY(Serialize, Name = "bVisible", Getter = IsVisible, Category = "Light", DisplayName = "Visible", Edit, Setter = SetVisible)
    bool mBVisible{true};
};
