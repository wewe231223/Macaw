#include "pch.h"
#include "World/ADirectionalLightActor.h"

ADirectionalLightActor::ADirectionalLightActor() {
    InitializeClass(StaticClass());
    mLightComponent = CreateDefaultComponent<UDirectionalLightComponent>(FName{"Light"});
    SetRootComponent(mLightComponent);
}

UDirectionalLightComponent* ADirectionalLightActor::GetLightComponent() const {
    return FindDefaultComponent(FName{"Light"}) == mLightComponent ? mLightComponent : nullptr;
}
