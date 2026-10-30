#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/USubUVComponent.h"

class UUVScrollComponent final : public USubUVComponent {
public:
    ReflectTypes(UUVScrollComponent, USubUVComponent);

public:
    UUVScrollComponent();
    ~UUVScrollComponent() override = default;

public:
    ReflectClass(UUVScrollComponent, EClassFlags::None);
    ReflectFunction(SetScrollSpeed, FName{"Speed"});

    void SetScrollSpeed(const FVector2& Speed);
    ReflectFunction(GetScrollSpeed);

    const FVector2& GetScrollSpeed() const;
    void Serialize(FArchive& Archive) override;

private:
    FUVTransform CalculateUVTransform() const override;
    bool CanAdvance() const override;
    void ResetPlaybackState() override;

private:
    ReflectAccessor(mScrollSpeed, GetScrollSpeed, SetScrollSpeed, "ScrollSpeed", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mScrollSpeed), ReflectEditor("UV Animation", "Scroll Speed"));

    FVector2 mScrollSpeed{0.1f, 0.1f};
    FVector2 mOffsetBase{};
    double mSpeedChangeTime{};

    ReflectMembers();
};
