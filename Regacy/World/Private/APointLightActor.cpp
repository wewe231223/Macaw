#include "pch.h"
#include "World/APointLightActor.h"

APointLightActor::APointLightActor() {
    InitializeClass(StaticClass());
    mLightComponent = CreateDefaultComponent<UPointLightComponent>(FName{"Light"});
    SetRootComponent(mLightComponent);
}

UPointLightComponent* APointLightActor::GetLightComponent() const {
    return FindDefaultComponent(FName{"Light"}) == mLightComponent ? mLightComponent : nullptr;
}
