#pragma once
#include "World/Component/USubUVComponent.h"
#include "World/Component/UUVScrollComponent.generated.h"

UCLASS()
class UUVScrollComponent final : public USubUVComponent {
public:
    GENERATED_TYPES(UUVScrollComponent)

public:
    UUVScrollComponent();
    ~UUVScrollComponent() override = default;

public:
    GENERATED_BODY(UUVScrollComponent)

    UFUNCTION()
    void SetScrollSpeed(const FVector2& Speed);
    UFUNCTION()
    const FVector2& GetScrollSpeed() const;
    void Serialize(FArchive& Archive) override;

private:
    FUVTransform CalculateUVTransform() const override;
    bool CanAdvance() const override;
    void ResetPlaybackState() override;

private:
    UPROPERTY(Serialize, Name = "ScrollSpeed", Getter = GetScrollSpeed, Category = "UV Animation", DisplayName = "Scroll Speed", Edit, Setter = SetScrollSpeed)
    FVector2 mScrollSpeed{0.1f, 0.1f};
    FVector2 mOffsetBase{};
    double mSpeedChangeTime{};
};
