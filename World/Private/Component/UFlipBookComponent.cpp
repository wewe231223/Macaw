#include "pch.h"
#include "World/Component/UFlipBookComponent.h"

#include <cmath>
#include <limits>

UFlipBookComponent::UFlipBookComponent() {
    InitializeClass(StaticClass());
    UpdateUVTransform();
    UpdateTickEnabled();
}

void UFlipBookComponent::SetSubImage(Int32 Columns, Int32 Rows, Int32 TotalFrames, float FrameRate, bool Looping) {
    mColumns = (std::max)(1, Columns);
    mRows = (std::max)(1, Rows);
    mTotalFrames = static_cast<Int32>(std::clamp<Int64>(TotalFrames, 1, (std::min)(static_cast<Int64>(mColumns) * mRows, static_cast<Int64>(std::numeric_limits<Int32>::max()))));
    mCurrentFrame = std::clamp(mCurrentFrame, 0, mTotalFrames - 1);
    SetFrameRate(FrameRate);
    SetLooping(Looping);
}

void UFlipBookComponent::SetFrameRate(float FrameRate) {
    if (!std::isfinite(FrameRate) || FrameRate < 0.0f) {
        return;
    }

    const double FramePosition{mFrameRate > 0.0f ? GetElapsedTime() * mFrameRate : static_cast<double>(mCurrentFrame)};

    mCurrentFrame = static_cast<Int32>((std::min)(std::floor(FramePosition), static_cast<double>(mTotalFrames - 1)));
    mFrameRate = FrameRate;
    SetElapsedTime(mFrameRate > 0.0f ? FramePosition / mFrameRate : 0.0);
}

void UFlipBookComponent::SetCurrentFrame(Int32 Frame) {
    mCurrentFrame = std::clamp(Frame, 0, mTotalFrames - 1);
    SetElapsedTime(mFrameRate > 0.0f ? static_cast<double>(mCurrentFrame) / mFrameRate : 0.0);
}

float UFlipBookComponent::GetFrameRate() const {
    return mFrameRate;
}

Int32 UFlipBookComponent::GetColumns() const {
    return mColumns;
}

Int32 UFlipBookComponent::GetRows() const {
    return mRows;
}

Int32 UFlipBookComponent::GetTotalFrames() const {
    return mTotalFrames;
}

Int32 UFlipBookComponent::GetCurrentFrame() const {
    if (mFrameRate <= 0.0f) {
        return mCurrentFrame;
    }

    return static_cast<Int32>((std::min)(std::floor(GetElapsedTime() * mFrameRate + 1e-9), static_cast<double>(mTotalFrames - 1)));
}

void UFlipBookComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);
    StaticClass()->SerializeDeclaredProperties(this, Archive);

    if (Archive.IsLoading()) {
        mCurrentFrame = 0;
        SetSubImage(mColumns, mRows, mTotalFrames, std::isfinite(mFrameRate) && mFrameRate >= 0.0f ? mFrameRate : 0.0f, IsLooping());
    }
}

FUVTransform UFlipBookComponent::CalculateUVTransform() const {
    const Int32 Frame{GetCurrentFrame()};
    const FVector2 Scale{1.0f / mColumns, 1.0f / mRows};

    return FUVTransform{Scale, FVector2{Frame % mColumns * Scale.mX, Frame / mColumns * Scale.mY}};
}

bool UFlipBookComponent::CanAdvance() const {
    return mTotalFrames > 1 && mFrameRate > 0.0f;
}

double UFlipBookComponent::GetDuration() const {
    return mFrameRate > 0.0f ? static_cast<double>(mTotalFrames) / mFrameRate : std::numeric_limits<double>::infinity();
}

void UFlipBookComponent::ResetPlaybackState() {
    mCurrentFrame = 0;
}
