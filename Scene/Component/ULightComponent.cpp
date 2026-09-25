#include "pch.h"

#include "ULightComponent.h"

#include "Scene/AActor.h"
#include "Scene/Subsystem/ULightSubsystem.h"
#include "Scene/UWorld.h"

void ULightComponent::MakeLightProbe(FLightProbe& OutProbe) const {
    OutProbe = FLightProbe{ .mColor = GetLightColor(), .mIntensity = GetIntensity(), .mPosition = GetComponentLocation(), .mDirection = GetComponentTransform().ToMatrixNoScale().Forward(), .mType = GetLightType()};
}

void ULightComponent::OnRegister() {
    ULightComponentBase::OnRegister();

    AActor* Owner{GetOwner()};
    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetLightSubsystem().RegisterComponent(this);
    }
}

void ULightComponent::OnUnregister() {
    AActor* Owner{GetOwner()};
    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetLightSubsystem().UnregisterComponent(this);
    }

    ULightComponentBase::OnUnregister();
}
