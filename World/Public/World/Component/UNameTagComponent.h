#pragma once
#include "World/Component/USceneComponent.h"
#include "Core/Asset/FAssetPath.h"
#include "RenderCore/FOverlayRenderData.h"
#include "Core/Base/FGuid.h"
#include "CoreUObject/TObjectRef.h"
#include "World/AActor.h"

class UNameTagComponent final : public USceneComponent {
public:
    UNameTagComponent() = default;
    ~UNameTagComponent() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UNameTagComponent, USceneComponent);

    void SetTargetActor(AActor* InTargetActor);
    AActor* GetTargetActor() const;
    void SetTargetLocalOffset(const FVector3& InOffset);
    const FVector3& GetTargetLocalOffset() const;
    FGuid GetObjectGuid() const;
    const FVector3& GetObjectOffset() const;

    void SetFontHandle(FAssetHandle FontHandle);
    FAssetHandle GetFontHandle() const;
    void SetText(const FString& Text);
    const FString& GetText() const;
    void SetColor(const FVector4& Color);
    const FVector4& GetColor() const;
    void SetPixelHeight(float PixelHeight);
    float GetPixelHeight() const;
    void SetLetterSpacing(float LetterSpacing);
    float GetLetterSpacing() const;
    void SetLineSpacing(float LineSpacing);
    float GetLineSpacing() const;
    void SetScreenOffset(const FVector2& ScreenOffset);
    const FVector2& GetScreenOffset() const;
    void SetVisible(bool Visible);
    bool IsVisible() const;

    bool MakeOverlayText(FOverlayTextProbe& OutProbe) const;
    bool ResolveLoadedReferences() override;
    void RefreshGuidText();
    void OnRegister() override;
    void OnUnregister() override;
    void Serialize(FArchive& Archive) override;

private:
    void RebuildTextGeometry();

private:
    TObjectRef<AActor> mTargetActor{};
    FGuid mExplicitTargetGuid{};
    FVector3 mTargetLocalOffset{};
    FAssetHandle mFontHandle{};
    FAssetPath mFontAssetPath{};
    FGuid mFontAssetGuid{};
    FString mText{};
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    float mPixelHeight{24.0f};
    float mLetterSpacing{};
    float mLineSpacing{};
    FVector2 mScreenOffset{0.0f, -8.0f};
    bool mVisible{true};
    TArray<FTextVertex> mVertices{};
};
