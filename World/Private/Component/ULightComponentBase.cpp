#include "pch.h"
#include <cfloat>
#include "World/Component/ULightComponentBase.h"

const FVector3& ULightComponentBase::GetLightColor() const {
    return mLightColor;
}

void ULightComponentBase::SetLightColor(const FVector3& InLightColor) {
    if (mLightColor == InLightColor) {
        return;
    }

    mLightColor = InLightColor;
    MarkRenderStateDirty();
}

float ULightComponentBase::GetIntensity() const {
    return mIntensity;
}

void ULightComponentBase::SetIntensity(float InIntensity) {
    const float Intensity{std::max(InIntensity, 0.0f)};

    if (mIntensity == Intensity) {
        return;
    }

    mIntensity = Intensity;
    MarkRenderStateDirty();
}

bool ULightComponentBase::IsVisible() const {
    return mBVisible;
}

void ULightComponentBase::SetVisible(bool BInVisible) {
    if (mBVisible == BInVisible) {
        return;
    }

    mBVisible = BInVisible;
    MarkRenderStateDirty();
}

void ULightComponentBase::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);

    Archive.Serialize("LightColor", mLightColor);
    Archive.Serialize("Intensity", mIntensity);
    Archive.Serialize("bVisible", mBVisible);

    if (Archive.IsLoading()) {
        MarkRenderStateDirty();
    }
}
