#pragma once

#include "PCH.h"
#include "ImGui/imgui.h"
#include "Core/Asset/FAssetHandle.h"
#include "Core/Base/FTransform.h"
#include "../EditorView/FAssetThumbnailRenderer.h"

class FAssetRegistry;
struct FTypeInfo;

struct FPropertyReferenceOption {
    const void* mId{nullptr};
    FString mLabel{};
    bool mBSelected{false};
    std::function<void()> mOnSelected{};
};

class FPropertyEditorContext {
public:
    bool BeginCategory(const char* Label, bool BDefaultOpen = true) const;
    void DrawDisabledText(const char* Text) const;
    void DrawButton(const char* Label, const std::function<void()>& OnClicked) const;

    void DrawBool(const char* Label, bool Value, const std::function<void(bool)>& Setter) const;
    void DrawFloat(const char* Label, float Value, float Speed, float Min, float Max, const std::function<void(float)>& Setter) const;
    void DrawVector2(const char* Label, const FVector2& Value, float Speed, float Min, float Max, const std::function<void(const FVector2&)>& Setter) const;
    void DrawVector3(const char* Label, const FVector3& Value, float Speed, float Min, float Max, const std::function<void(const FVector3&)>& Setter) const;
    void DrawColor(const char* Label, const FVector4& Value, const std::function<void(const FVector4&)>& Setter) const;
    void DrawText(const char* Label, const FString& Value, const std::function<void(const FString&)>& Setter) const;
    void DrawTransform(const char* Label, const FTransform& Value, const std::function<void(const FTransform&)>& Setter);

    void DrawReferencePicker(const char* Label, const char* Preview, bool BNoneSelected, const std::function<void()>& ClearSelection, const std::vector<FPropertyReferenceOption>& Options) const;
    void DrawAssetPicker(const char* Label, FAssetRegistry& Registry, const FTypeInfo& AssetType, FAssetHandle CurrentHandle, const std::function<void(FAssetHandle)>& Setter) const;

    void BindThumbnailRenderer(FAssetThumbnailRenderer* InThumbnailRenderer);

private:
    void UpdateTransformFields(const FTransform& Transform);
    FTransform BuildDesiredTransform() const;

    bool SupportsAssetThumbnail(const FTypeInfo& AssetType) const;

    ID3D11ShaderResourceView* GetAssetThumbnail(FAssetRegistry& Registry, FAssetHandle AssetHandle) const;

    bool DrawAssetOption(const char* Label, ID3D11ShaderResourceView* Thumbnail, bool BSelected) const;

private:
    FAssetThumbnailRenderer* mThumbnailRenderer{nullptr};

    ImGuiID mEditingTransformId{0};
    FVector3 mEditPosition{};
    FRotator mEditRotation{};
    FVector3 mEditScale{1.0f, 1.0f, 1.0f};
    bool mBAbsoluteLocation{false};
    bool mBAbsoluteRotation{false};
    bool mBAbsoluteScale{false};
};
