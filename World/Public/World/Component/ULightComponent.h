#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/ULightComponentBase.h"
#include "RenderCore/FLightSceneProxy.h"

class ULightComponent : public ULightComponentBase {
public:
    ReflectTypes(ULightComponent, ULightComponentBase);

public:
    ULightComponent();
    ~ULightComponent() override = default;

public:
    ReflectClass(ULightComponent, EClassFlags::Abstract);
    ReflectFunction(GetLightType);
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

private:

    ReflectMembers();
};
