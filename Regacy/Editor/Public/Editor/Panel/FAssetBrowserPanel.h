/**
 * @file FAssetBrowserPanel.h
 * @brief 전역 에셋 저장소를 사용하는 에셋 브라우저 패널을 정의합니다.
 */
#pragma once
#include "Editor/Panel/FEditorWindow.h"
#include "Asset/FAssetEntry.h"
#include "Editor/View/FAssetThumbnailRenderer.h"

#include "ImGui/imgui.h"
#include <functional>
#include "Editor/World/FWorldEditorContext.h"

class FAssetBrowserPanel : public FEditorWindow {
public:
    /**
     * @brief 전역 에셋 서비스를 사용하는 브라우저를 생성합니다.
     * @param InEditorContext 에셋을 배치할 월드의 편집 컨텍스트입니다.
     * @param InThumbnailRenderer 선택적인 썸네일 생성기입니다.
     * @param InOpenMaterialEditor 머티리얼을 열 때 실행할 콜백입니다.
     * @pre 패널 사용 중에는 활성 엔진의 에셋 서비스가 존재해야 합니다.
     */
    explicit FAssetBrowserPanel(FWorldEditorContext& InEditorContext, FAssetThumbnailRenderer* InThumbnailRenderer, std::function<void(FAssetHandle)> InOpenMaterialEditor);

public:
    void BeginExternalDropFrame();
    bool HandleExternalFileDrop(const std::filesystem::path& FilePath, const ImVec2& ScreenPosition);

private:
    void DrawContents() override;
    void PushWindowStyle() override;
    void PopWindowStyle() override;

    static FString GetParentFolder(const FString& AssetPath);
    static const char* GetAssetTypeLabel(EAssetType AssetType);

    bool IsInSelectedFolder(const FAssetEntry& Entry) const;
    void DrawFolderTree(const FString& FolderPath, const char* FolderName);
    void DrawAssetTile(const FAssetEntry& Entry);

private:
    FWorldEditorContext& mEditorContext;
    FString mSelectedFolder{"/Game"};
    FAssetHandle mSelectedAsset{};
    ImGuiTextFilter mAssetFilter{};
    EAssetType mSelectedAssetType{EAssetType::END};
    ImVec2 mDropTargetMin{};
    ImVec2 mDropTargetMax{};
    bool mBDropTargetActive{false};

    FAssetThumbnailRenderer* mThumbnailRenderer{nullptr};
    std::function<void(FAssetHandle)> mOpenMaterialEditor{};
};
