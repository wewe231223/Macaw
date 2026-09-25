#pragma once
#include "Scene/Component/UBillboardComponent.h"

#include "Core/Asset/FAssetHandle.h"
#include "Core/Base/FRenderProbe.h"

#include "../../Serialize/FArchive.h"

class FPropertyEditorContext;

class USubUVComponent : public UBillboardComponent {
public:
    USubUVComponent() = default;
    ~USubUVComponent() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(USubUVComponent, UBillboardComponent);

    // SubUV
    void SetSubImage(Int32 InHorizontal, Int32 InVertical, Int32 InTotalFrame, float InFrameRate, bool BInLooping);
    void SetFrameRate(float InFrameRate);
    void SetCurrentFrame(Int32 InFrame);

    void PlaySubUV();
    void PauseSubUV();
    void StopSubUV();
    void RestartSubUV();

    bool IsPlaying() const;
    bool IsLooping() const;

    void UpdateUVFromCurrentFrame();

    void Tick(float DeltaTime) override;

    void DrawPanels(FPropertyEditorContext& Context) override;

protected:
    void Serialize(FArchive& Archive) override;

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
