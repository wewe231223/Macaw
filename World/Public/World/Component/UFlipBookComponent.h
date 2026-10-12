#pragma once
#include "World/Component/USubUVComponent.h"
#include "World/Component/UFlipBookComponent.generated.h"

UCLASS()
class UFlipBookComponent final : public USubUVComponent {
public:
    GENERATED_TYPES(UFlipBookComponent)

public:
    UFlipBookComponent();
    ~UFlipBookComponent() override = default;

public:
    GENERATED_BODY(UFlipBookComponent)

    void SetSubImage(Int32 Columns, Int32 Rows, Int32 TotalFrames, float FrameRate, bool Looping);
    UFUNCTION()
    void SetFrameRate(float FrameRate);
    UFUNCTION()
    void SetCurrentFrame(Int32 Frame);
    UFUNCTION()
    float GetFrameRate() const;
    void SetColumns(Int32 Columns);
    void SetRows(Int32 Rows);
    void SetTotalFrames(Int32 TotalFrames);
    Int32 GetMaximumTotalFrames() const;
    Int32 GetMaximumCurrentFrame() const;
    Int32 GetColumns() const;
    Int32 GetRows() const;
    Int32 GetTotalFrames() const;
    UFUNCTION()
    Int32 GetCurrentFrame() const;
    void Serialize(FArchive& Archive) override;

private:
    FUVTransform CalculateUVTransform() const override;
    bool CanAdvance() const override;
    double GetDuration() const override;
    void ResetPlaybackState() override;

private:
    UPROPERTY(Serialize, Name = "SubImageHorizontal", Getter = GetColumns, Category = "Flipbook", DisplayName = "Columns", Edit, Setter = SetColumns, Min = 1, Speed = 1.0f)
    Int32 mColumns{1};
    UPROPERTY(Serialize, Name = "SubImageVertical", Getter = GetRows, Category = "Flipbook", DisplayName = "Rows", Edit, Setter = SetRows, Min = 1, Speed = 1.0f)
    Int32 mRows{1};
    UPROPERTY(Serialize, Name = "TotalFrame", Getter = GetTotalFrames, Category = "Flipbook", DisplayName = "Total Frames", Edit, Setter = SetTotalFrames, Min = 1, MaxGetter = GetMaximumTotalFrames, Speed = 1.0f)
    Int32 mTotalFrames{1};
    UPROPERTY(Edit, Transient, Name = "CurrentFrame", Category = "Flipbook", DisplayName = "Current Frame", Getter = GetCurrentFrame, Setter = SetCurrentFrame, Min = 0, MaxGetter = GetMaximumCurrentFrame, Speed = 1.0f)
    Int32 mCurrentFrame{};
    UPROPERTY(Serialize, Name = "FrameRate", Getter = GetFrameRate, Category = "Flipbook", DisplayName = "Frame Rate", Edit, Setter = SetFrameRate, Min = 0.0f, Max = 240.0f)
    float mFrameRate{30.0f};
};
