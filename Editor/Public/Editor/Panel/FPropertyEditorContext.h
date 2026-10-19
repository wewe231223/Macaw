/**
 * @file FPropertyEditorContext.h
 * @brief 속성 UI의 편집 상태와 전역 에셋 선택 기능을 정의합니다.
 */
#pragma once
#include "Editor/Property/IPropertyEditorContext.h"
#include "ImGui/imgui.h"
#include "Editor/View/FAssetThumbnailRenderer.h"
#include "Editor/Property/FRotationEditorState.h"
#include <d3d11.h>
#include <unordered_map>

class FPropertyEditorContext final : public IPropertyEditorContext {
public:
    FPropertyEditorContext() = default;
    ~FPropertyEditorContext() override = default;

public:
    void BeginProperty(const char* Label) const override;
    void EndProperty() const override;
    bool BeginCategory(const char* Label, bool BDefaultOpen = true) const override;
    void DrawDisabledText(const char* Text) const override;
    void DrawButton(const char* Label, const std::function<void()>& OnClicked) const override;

    void DrawBool(const char* Label, bool Value, const std::function<void(bool)>& Setter) const override;
    void DrawInteger(const char* Label, Int64 Value, float Speed, Int64 Min, Int64 Max, const std::function<void(Int64)>& Setter) const override;
    void DrawFloat(const char* Label, float Value, float Speed, float Min, float Max, const std::function<void(float)>& Setter) const override;
    void DrawVector2(const char* Label, const FVector2& Value, float Speed, float Min, float Max, const std::function<void(const FVector2&)>& Setter) const override;
    void DrawVector3(const char* Label, const FVector3& Value, float Speed, float Min, float Max, const std::function<void(const FVector3&)>& Setter) const override;
    void DrawColor(const char* Label, const FVector4& Value, const std::function<void(const FVector4&)>& Setter) const override;
    void DrawText(const char* Label, const FString& Value, const std::function<void(const FString&)>& Setter) const override;
    void DrawTransform(const char* Label, const FTransform& Value, const std::function<void(const FTransform&)>& Setter) override;

    void DrawReferencePicker(const char* Label, const char* Preview, bool BNoneSelected, const std::function<void()>& ClearSelection, const std::vector<FPropertyReferenceOption>& Options) const override;
    /**
     * @brief 전역 에셋 서비스를 사용해 타입별 에셋 선택 UI를 표시합니다.
     * @param Label 속성 이름입니다.
     * @param AssetType 허용할 에셋 클래스입니다.
     * @param CurrentHandle 현재 선택된 핸들입니다.
     * @param Setter 선택 변경 시 실행할 함수입니다.
     * @note 활성 에셋 서비스가 없으면 비활성 안내 문구를 표시합니다.
     */
    void DrawAssetPicker(const char* Label, const UClass& AssetType, FAssetHandle CurrentHandle, const std::function<void(FAssetHandle)>& Setter) const override;

    void BindThumbnailRenderer(FAssetThumbnailRenderer* InThumbnailRenderer);

private:
    bool SupportsAssetThumbnail(const UClass& AssetType) const;

    bool DrawAssetOption(const char* Label, ID3D11ShaderResourceView* Thumbnail, bool BSelected) const;

private:
    FAssetThumbnailRenderer* mThumbnailRenderer{nullptr};

    std::unordered_map<ImGuiID, FRotationEditorState> mRotationEdits{};
};
