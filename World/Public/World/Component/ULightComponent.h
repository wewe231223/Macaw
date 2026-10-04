#pragma once
#include "World/Component/ULightComponentBase.h"
#include "RenderCore/FRenderProbe.h"

class ULightComponent : public ULightComponentBase {
public:
    ULightComponent() = default;
    ~ULightComponent() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(ULightComponent, ULightComponentBase)

    virtual ELightType GetLightType() const = 0;
    virtual void MakeLightProbe(FLightProbe& OutProbe) const;

    void OnRegister() override;
    void OnUnregister() override;
};
