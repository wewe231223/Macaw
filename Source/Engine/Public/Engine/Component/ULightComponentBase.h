#pragma once

#include "Engine/Component/USceneComponent.h"
#include "Engine/FLightSceneProxy.h"

class FPrimitiveDrawInterface;
class FAssetManager;

class ULightComponentBase : public USceneComponent {
public:
    ReflectTypes(ULightComponentBase, USceneComponent);

public:
    ULightComponentBase();
    ~ULightComponentBase() override;

public:
    ReflectClass(ULightComponentBase, EClassFlags::NoFactory);

    float GetIntensity() const;
    bool SetIntensity(float Intensity);
    const FVector4& GetLightColor() const;
    bool SetLightColor(const FVector4& Color);
    bool IsVisible() const;
    bool SetVisible(bool Visible);

    virtual FLightSceneProxy CreateLightProxy() const;
    void SynchronizeLight();
    bool SetTransform(const FTransform& Transform) override;
    void UnregisterComponent() override;
    void DrawVisualization(FPrimitiveDrawInterface& Draw, bool Selected) const;

private:
    ReflectMembers();
    void OnRegister() override;
    void ResolveIcon(FAssetManager* Assets);

private:
    float mIntensity{1.0f};
    ReflectAccessor(mIntensity, GetIntensity, SetIntensity, "Intensity", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    FVector4 mLightColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectAccessor(mLightColor, GetLightColor, SetLightColor, "LightColor", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    bool mVisible{true};
    ReflectAccessor(mVisible, IsVisible, SetVisible, "Visible", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    UTexture2D* mIcon{};
    ReflectProperty(mIcon, "Icon", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FAssetManager* mIconAssets{};
    ReflectProperty(mIconAssets, "IconAssets", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::unique_ptr<UTexture2D> mFallbackIcon{};
    ReflectProperty(mFallbackIcon, "FallbackIcon", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
