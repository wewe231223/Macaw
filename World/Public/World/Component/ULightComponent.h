#pragma once
#include "World/Component/ULightComponentBase.h"
#include "RenderCore/FLightSceneProxy.h"
#include "World/Component/ULightComponent.generated.h"

UCLASS(Abstract)
class ULightComponent : public ULightComponentBase {
public:
    GENERATED_TYPES(ULightComponent)

public:
    ULightComponent();
    ~ULightComponent() override = default;

public:
    GENERATED_BODY(ULightComponent)

    UFUNCTION()
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
