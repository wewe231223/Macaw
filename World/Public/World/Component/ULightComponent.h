#pragma once
#include "World/Component/ULightComponentBase.h"
#include "RenderCore/FLightSceneProxy.h"

class ULightComponent : public ULightComponentBase {
public:
    ULightComponent() = default;
    ~ULightComponent() override = default;

public:
    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(ULightComponent, ULightComponentBase)

    virtual ELightType GetLightType() const = 0;
    virtual void BuildLightShaderParameters(FLightShaderParameters& OutParameters) const;

    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FLightSceneProxy> CreateSceneProxy() const;
    void CreateRenderState() override;
    void DestroyRenderState() override;
    void SendRenderTransform() override;
    void OnTransformUpdate() override;

    void OnRegister() override;
    void OnUnregister() override;
};
