#pragma once
#include "World/Component/UPrimitiveComponent.h"
#include "Core/Base/FAssetHandle.h"
#include "Core/Asset/FAssetPath.h"
#include "RenderCore/FRenderData.h"
#include "Core/STL.h"
#include "Asset/UFont.h"
#include "Asset/Pipeline/UPipeline.h"
#include "World/Component/UBillboardTextComponent.generated.h"

UCLASS()
class UBillboardTextComponent : public UPrimitiveComponent {
public:
    GENERATED_TYPES(UBillboardTextComponent)

public:
    UBillboardTextComponent();
    ~UBillboardTextComponent() override = default;

public:
    GENERATED_BODY(UBillboardTextComponent)

    void SetFontHandle(FAssetHandle InFontHandle);
    void SetPipelineHandle(FAssetHandle InPipelineHandle);

    void SetText(const FString& InText);
    void SetColor(const FVector4& InColor);

    void SetCharacterHeight(float InCharacterHeight);
    void SetLetterSpacing(float InLetterSpacing);
    void SetLineSpacing(float InLineSpacing);

    FAssetHandle GetFontHandle() const;
    FAssetHandle GetPipelineHandle() const;

    const FString& GetText() const;
    const FVector4& GetColor() const;

    float GetCharacterHeight() const;
    float GetLetterSpacing() const;
    float GetLineSpacing() const;

    const TArray<FTextVertex>& GetVertices() const;

    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const override;
    void SendRenderTransform() override;

    void OnRegister() override;

protected:
    virtual bool TryGetTextWorld(FMatrix& OutWorld) const;
    void Serialize(FArchive& Archive) override;

    // Dynamic Font에서 글리프를 요청하고 FTextVertex 배열을 다시 생성한다.
    void RebuildTextGeometry();

protected:
    UPROPERTY(Edit, Serialize, Asset, AssetClass = UFont, Getter = GetFontHandle, Setter = SetFontHandle, AssetGuid = mFontAssetGuid, AssetPath = mFontAssetPath, Category = "Text Assets", DisplayName = "Font")
    FAssetHandle mFontHandle{};
    UPROPERTY(Edit, Serialize, Asset, AssetClass = UPipeline, Getter = GetPipelineHandle, Setter = SetPipelineHandle, AssetGuid = mPipelineAssetGuid, AssetPath = mPipelineAssetPath, Category = "Text Assets", DisplayName = "Pipeline")
    FAssetHandle mPipelineHandle{};
    FAssetPath mFontAssetPath{};
    FAssetPath mPipelineAssetPath{};
    FGuid mFontAssetGuid{};
    FGuid mPipelineAssetGuid{};

    UPROPERTY(Serialize, Name = "Text", Getter = GetText, Category = "Text", DisplayName = "Text", Edit, Setter = SetText)
    FString mText{};

    UPROPERTY(Serialize, Name = "Color", Getter = GetColor, Category = "Text", DisplayName = "Color", Edit, Setter = SetColor, Color)
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};

    UPROPERTY(Serialize, Name = "CharacterHeight", Getter = GetCharacterHeight, Category = "Text", DisplayName = "Character Height", Edit, Setter = SetCharacterHeight, Min = 0.001f)
    float mCharacterHeight{1.0f};
    UPROPERTY(Serialize, Name = "LetterSpacing", Getter = GetLetterSpacing, Category = "Text", DisplayName = "Letter Spacing", Edit, Setter = SetLetterSpacing)
    float mLetterSpacing{0.0f};
    UPROPERTY(Serialize, Name = "LineSpacing", Getter = GetLineSpacing, Category = "Text", DisplayName = "Line Spacing", Edit, Setter = SetLineSpacing)
    float mLineSpacing{0.0f};

    TArray<FTextVertex> mVertices{};
};
