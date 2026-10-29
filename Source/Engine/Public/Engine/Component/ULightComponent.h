#pragma once

#include "Engine/Component/ULightComponentBase.h"

class ULightComponent : public ULightComponentBase {
public:
    ReflectTypes(ULightComponent, ULightComponentBase);

public:
    ULightComponent();
    ~ULightComponent() override;

public:
    ReflectClass(ULightComponent, EClassFlags::NoFactory);

    bool GetCastShadows() const;
    bool SetCastShadows(bool CastShadows);
    Uint32 GetShadowResolution() const;
    bool SetShadowResolution(Uint32 Resolution);
    float GetShadowResolutionScale() const;
    bool SetShadowResolutionScale(float Scale);
    float GetShadowBias() const;
    bool SetShadowBias(float Bias);
    float GetShadowSlopeBias() const;
    bool SetShadowSlopeBias(float Bias);
    float GetShadowSharpen() const;
    bool SetShadowSharpen(float Sharpen);

    FLightSceneProxy CreateLightProxy() const override;

private:
    ReflectMembers();

private:
    bool mCastShadows{true};
    ReflectAccessor(mCastShadows, GetCastShadows, SetCastShadows, "CastShadows", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    Uint32 mShadowResolution{1024};
    ReflectAccessor(mShadowResolution, GetShadowResolution, SetShadowResolution, "ShadowResolution", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mShadowResolutionScale{1.0f};
    ReflectAccessor(mShadowResolutionScale, GetShadowResolutionScale, SetShadowResolutionScale, "ShadowResolutionScale", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mShadowBias{0.0005f};
    ReflectAccessor(mShadowBias, GetShadowBias, SetShadowBias, "ShadowBias", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mShadowSlopeBias{1.0f};
    ReflectAccessor(mShadowSlopeBias, GetShadowSlopeBias, SetShadowSlopeBias, "ShadowSlopeBias", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mShadowSharpen{};
    ReflectAccessor(mShadowSharpen, GetShadowSharpen, SetShadowSharpen, "ShadowSharpen", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
