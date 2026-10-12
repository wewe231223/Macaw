#pragma once
#include "World/Component/USubUVComponent.h"

class UUVScrollComponent final : public USubUVComponent {
public:
    UUVScrollComponent();
    ~UUVScrollComponent() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UUVScrollComponent, USubUVComponent);

    void SetScrollSpeed(const FVector2& Speed);
    const FVector2& GetScrollSpeed() const;
    void Serialize(FArchive& Archive) override;

private:
    FUVTransform CalculateUVTransform() const override;
    bool CanAdvance() const override;
    void ResetPlaybackState() override;

private:
    FVector2 mScrollSpeed{0.1f, 0.1f};
    FVector2 mOffsetBase{};
    double mSpeedChangeTime{};
};
