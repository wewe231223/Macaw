#pragma once
#include "World/Component/UBillboardComponent.h"
#include "Core/Archive/FArchive.h"

class UScrollUVComponent final : public UBillboardComponent {
public:
    UScrollUVComponent();
    ~UScrollUVComponent() override = default;

    UScrollUVComponent(const UScrollUVComponent&) = delete;
    UScrollUVComponent& operator=(const UScrollUVComponent&) = delete;

    UScrollUVComponent(const UScrollUVComponent&&) = delete;
    UScrollUVComponent& operator=(const UScrollUVComponent&&) = delete;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UScrollUVComponent, UBillboardComponent);

    FVector2 GetScrollSpeed() const;

    bool IsPlaying() const;

    bool IsLooping() const;

    void PlayScrollUV();

    void PauseScrollUV();

    void SetScrollSpeed(FVector2 InScrollSpeed);

    /// <summary>재생 중인 UV 스크롤의 이동량을 누적하고 표시 좌표를 갱신합니다.</summary>
    void TickComponent(float DeltaTime) override;
    void UpdateUVFromCurrentFrame();

protected:
    void Serialize(FArchive& Archive) override;

private:
    void UpdateTickEnabled();

private:
    FVector2 mScrollSpeed{0.1f, 0.1f};   // 초당 UV 이동량
    FVector2 mCurrentOffset{0.0f, 0.0f}; // 누적값
    bool mBPlaying{true};
    bool mBLooping{true};
};
