#include "pch.h"
#include "World/Component/UUVScrollComponent.h"

#include <cmath>

UUVScrollComponent::UUVScrollComponent() {
    InitializeClass(StaticClass());
    UpdateTickEnabled();
}

void UUVScrollComponent::SetScrollSpeed(const FVector2& Speed) {
    if (!std::isfinite(Speed.mX) || !std::isfinite(Speed.mY)) {
        return;
    }

    mOffsetBase = CalculateUVTransform().mOffset;
    mSpeedChangeTime = GetElapsedTime();
    mScrollSpeed = Speed;
    UpdateUVTransform();
    UpdateTickEnabled();
}

const FVector2& UUVScrollComponent::GetScrollSpeed() const {
    return mScrollSpeed;
}

void UUVScrollComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    if (Archive.IsLoading()) {
        ResetPlaybackState();
        if (!std::isfinite(mScrollSpeed.mX) || !std::isfinite(mScrollSpeed.mY)) {
            mScrollSpeed = {};
        }

        UpdateUVTransform();
        UpdateTickEnabled();
    }
}

FUVTransform UUVScrollComponent::CalculateUVTransform() const {
    double X{mOffsetBase.mX + (GetElapsedTime() - mSpeedChangeTime) * mScrollSpeed.mX};
    double Y{mOffsetBase.mY + (GetElapsedTime() - mSpeedChangeTime) * mScrollSpeed.mY};

    if (IsLooping()) {
        X -= std::floor(X);
        Y -= std::floor(Y);
    }

    return FUVTransform{FVector2{1.0f, 1.0f}, FVector2{static_cast<float>(X), static_cast<float>(Y)}};
}

bool UUVScrollComponent::CanAdvance() const {
    return mScrollSpeed.mX != 0.0f || mScrollSpeed.mY != 0.0f;
}

void UUVScrollComponent::ResetPlaybackState() {
    mOffsetBase = {};
    mSpeedChangeTime = 0.0;
}
