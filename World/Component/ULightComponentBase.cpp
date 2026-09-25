#include "pch.h"

#include "ULightComponentBase.h"

const FVector3& ULightComponentBase::GetLightColor() const {
    return mLightColor;
}

void ULightComponentBase::SetLightColor(const FVector3& InLightColor) {
    mLightColor = InLightColor;
}

float ULightComponentBase::GetIntensity() const {
    return mIntensity;
}

void ULightComponentBase::SetIntensity(float InIntensity) {
    mIntensity = std::max(InIntensity, 0.0f);
}

bool ULightComponentBase::IsVisible() const {
    return mBVisible;
}

void ULightComponentBase::SetVisible(bool BInVisible) {
    mBVisible = BInVisible;
}

void ULightComponentBase::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);

    Archive.Serialize("LightColor", mLightColor);
    Archive.Serialize("Intensity", mIntensity);
    Archive.Serialize("bVisible", mBVisible);
}
