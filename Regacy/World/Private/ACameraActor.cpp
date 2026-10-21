#include "pch.h"
#include "World/ACameraActor.h"

ACameraActor::ACameraActor() {
    InitializeClass(StaticClass());
    mCameraComponent = CreateDefaultComponent<UCameraComponent>(FName{"Camera"});
    SetRootComponent(mCameraComponent);
}

UCameraComponent* ACameraActor::GetCameraComponent() const {
    return FindDefaultComponent(FName{"Camera"}) == mCameraComponent ? mCameraComponent : nullptr;
}
