#pragma once
#include "World/Component/UBillboardComponent.h"
#include "Core/Base/FAssetHandle.h"
#include "RenderCore/FRenderData.h"
#include "Core/Archive/FArchive.h"

class USubUVComponent : public UBillboardComponent {
public:
    USubUVComponent();
    ~USubUVComponent() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(USubUVComponent, UBillboardComponent);

    // SubUV
    void SetSubImage(Int32 InHorizontal, Int32 InVertical, Int32 InTotalFrame, float InFrameRate, bool BInLooping);
    void SetFrameRate(float InFrameRate);
    void SetCurrentFrame(Int32 InFrame);

    void PlaySubUV();
    void PauseSubUV();
    void StopSubUV();
    void RestartSubUV();

    float GetFrameRate() const;
    Int32 GetSubImageHorizontal() const;
    Int32 GetSubImageVertical() const;
    Int32 GetTotalFrame() const;

    bool IsPlaying() const;
    bool IsLooping() const;

    void UpdateUVFromCurrentFrame();

    /// <summary>경과 시간에 따라 스프라이트 시트의 재생 프레임과 UV를 갱신합니다.</summary>
    void TickComponent(float DeltaTime) override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    void UpdateTickEnabled();

private:
    Int32 mSubImageHorizontal{1};
    Int32 mSubImageVertical{1};
    Int32 mTotalFrame{1};
    float mFrameRate{30.0f};
    bool mBLooping{true};
    bool mBPlaying{true};

    float mElapsedTime{0.0f};
    Int32 mCurrentFrameIndex{0};
};
