#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UPrimitiveComponent.h"
#include "Core/Asset/FAssetReference.h"
#include "RenderCore/FRenderData.h"
#include "Core/STL.h"
#include "Asset/UFont.h"
#include "Asset/Pipeline/UPipeline.h"

class FProperty;

class UBillboardTextComponent : public UPrimitiveComponent {
public:
    ReflectTypes(UBillboardTextComponent, UPrimitiveComponent);

public:
    UBillboardTextComponent();
    ~UBillboardTextComponent() override = default;

public:
    ReflectClass(UBillboardTextComponent, EClassFlags::None);

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

    /** @brief 전역 에셋 서비스에서 기본 폰트와 파이프라인을 조회하고 이 컴포넌트의 텍스트 정점을 갱신합니다. */
    void OnRegister() override;

private:
    virtual bool TryGetTextWorld(FMatrix& OutWorld) const;

    void Serialize(FArchive& Archive) override;

    // Dynamic Font에서 글리프를 요청하고 FTextVertex 배열을 다시 생성한다.
    void RebuildTextGeometry();

    static void SerializeFontHandleProperty(void* Container, FArchive& Archive, const FProperty& Property);
    static void SerializePipelineHandleProperty(void* Container, FArchive& Archive, const FProperty& Property);
    FAssetHandle GetRenderFontHandle() const;
    FAssetHandle GetRenderPipelineHandle() const;

private:
    ReflectAccessor(mFontReference, GetFontHandle, SetFontHandle, "FontHandle", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Asset, &SerializeFontHandleProperty, ReflectEditor("Text Assets", "Font"), ReflectAsset(UFont));

    FAssetReference mFontReference{};
    ReflectAccessor(mPipelineReference, GetPipelineHandle, SetPipelineHandle, "PipelineHandle", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Asset, &SerializePipelineHandleProperty, ReflectEditor("Text Assets", "Pipeline"), ReflectAsset(UPipeline));

    FAssetReference mPipelineReference{};
    ReflectAccessor(mText, GetText, SetText, "Text", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mText), ReflectEditor("Text", "Text"));

    FString mText{};
    ReflectAccessor(mColor, GetColor, SetColor, "Color", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Color, ReflectSerializer(mColor), ReflectEditor("Text", "Color"));

    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectAccessor(mCharacterHeight, GetCharacterHeight, SetCharacterHeight, "CharacterHeight", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mCharacterHeight), ReflectEditor("Text", "Character Height"), ReflectRange(0.001f, std::numeric_limits<float>::max()));

    float mCharacterHeight{1.0f};
    ReflectAccessor(mLetterSpacing, GetLetterSpacing, SetLetterSpacing, "LetterSpacing", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mLetterSpacing), ReflectEditor("Text", "Letter Spacing"));

    float mLetterSpacing{0.0f};
    ReflectAccessor(mLineSpacing, GetLineSpacing, SetLineSpacing, "LineSpacing", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mLineSpacing), ReflectEditor("Text", "Line Spacing"));

    float mLineSpacing{0.0f};

    TArray<FTextVertex> mVertices{};

    ReflectMembers();
};
