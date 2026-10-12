#pragma once
#include "World/Component/USubUVComponent.h"

class UFlipBookComponent final : public USubUVComponent {
public:
    UFlipBookComponent();
    ~UFlipBookComponent() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UFlipBookComponent, USubUVComponent);

    void SetSubImage(Int32 Columns, Int32 Rows, Int32 TotalFrames, float FrameRate, bool Looping);
    void SetFrameRate(float FrameRate);
    void SetCurrentFrame(Int32 Frame);
    float GetFrameRate() const;
    Int32 GetColumns() const;
    Int32 GetRows() const;
    Int32 GetTotalFrames() const;
    Int32 GetCurrentFrame() const;
    void Serialize(FArchive& Archive) override;

private:
    FUVTransform CalculateUVTransform() const override;
    bool CanAdvance() const override;
    double GetDuration() const override;
    void ResetPlaybackState() override;

private:
    Int32 mColumns{1};
    Int32 mRows{1};
    Int32 mTotalFrames{1};
    Int32 mCurrentFrame{};
    float mFrameRate{30.0f};
};
