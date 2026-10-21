#include "pch.h"
#include "World/ASpotLightActor.h"

ASpotLightActor::ASpotLightActor() {
    InitializeClass(StaticClass());
    mLightComponent = CreateDefaultComponent<USpotLightComponent>(FName{"Light"});
    SetRootComponent(mLightComponent);
}

USpotLightComponent* ASpotLightActor::GetLightComponent() const {
    return FindDefaultComponent(FName{"Light"}) == mLightComponent ? mLightComponent : nullptr;
}
