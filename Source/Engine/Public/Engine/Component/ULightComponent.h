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
private:
    ReflectMembers();
};
