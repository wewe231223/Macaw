#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/USubUVComponent.h"

class UFlipBookComponent final : public USubUVComponent {
public:
    ReflectTypes(UFlipBookComponent, USubUVComponent);

public:
    UFlipBookComponent();
    ~UFlipBookComponent() override = default;

public:
    ReflectClass(UFlipBookComponent, EClassFlags::None);

    void SetSubImage(Int32 Columns, Int32 Rows, Int32 TotalFrames, float FrameRate, bool Looping);
    ReflectFunction(SetFrameRate, FName{"FrameRate"});
    void SetFrameRate(float FrameRate);
    ReflectFunction(SetCurrentFrame, FName{"Frame"});
    void SetCurrentFrame(Int32 Frame);
    ReflectFunction(GetFrameRate);
    float GetFrameRate() const;
    void SetColumns(Int32 Columns);
    void SetRows(Int32 Rows);
    void SetTotalFrames(Int32 TotalFrames);
    Int32 GetMaximumTotalFrames() const;
    Int32 GetMaximumCurrentFrame() const;
    Int32 GetColumns() const;
    Int32 GetRows() const;
    Int32 GetTotalFrames() const;
    ReflectFunction(GetCurrentFrame);
    Int32 GetCurrentFrame() const;
    void Serialize(FArchive& Archive) override;

private:
    FUVTransform CalculateUVTransform() const override;
    bool CanAdvance() const override;
    double GetDuration() const override;
    void ResetPlaybackState() override;

private:
    ReflectAccessor(mColumns, GetColumns, SetColumns, "SubImageHorizontal", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mColumns), ReflectEditor("Flipbook", "Columns"), ReflectRange(1, std::numeric_limits<float>::max()), ReflectSpeed(1.0f));
    Int32 mColumns{1};
    ReflectAccessor(mRows, GetRows, SetRows, "SubImageVertical", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mRows), ReflectEditor("Flipbook", "Rows"), ReflectRange(1, std::numeric_limits<float>::max()), ReflectSpeed(1.0f));
    Int32 mRows{1};
    ReflectAccessor(mTotalFrames, GetTotalFrames, SetTotalFrames, "TotalFrame", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mTotalFrames), ReflectEditor("Flipbook", "Total Frames"), ReflectRange(1, std::numeric_limits<float>::max()), ReflectSpeed(1.0f), ReflectBounds(nullptr, ReflectBound(GetMaximumTotalFrames)));
    Int32 mTotalFrames{1};
    ReflectAccessor(mCurrentFrame, GetCurrentFrame, SetCurrentFrame, "CurrentFrame", EPropertyFlags::Edit | EPropertyFlags::Transient, ReflectEditor("Flipbook", "Current Frame"), ReflectRange(0, std::numeric_limits<float>::max()), ReflectSpeed(1.0f), ReflectBounds(nullptr, ReflectBound(GetMaximumCurrentFrame)));
    Int32 mCurrentFrame{};
    ReflectAccessor(mFrameRate, GetFrameRate, SetFrameRate, "FrameRate", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mFrameRate), ReflectEditor("Flipbook", "Frame Rate"), ReflectRange(0.0f, 240.0f));
    float mFrameRate{30.0f};

    ReflectMembers();
};
