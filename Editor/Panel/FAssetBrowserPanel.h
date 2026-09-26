#pragma once

#include "Editor/Panel/FEditorWindow.h"
#include "Asset/FAssetEntry.h"
#include "Editor/View/FAssetThumbnailRenderer.h"

#include "ImGui/imgui.h"
#include <functional>
#include "World/FWorldEditorContext.h"
#include "Asset/FAssetRegistry.h"

class FAssetBrowserPanel : public FEditorWindow {
public:
    explicit FAssetBrowserPanel(FAssetRegistry& InAssetRegistry, FWorldEditorContext& InEditorContext, FAssetThumbnailRenderer* InThumbnailRenderer, std::function<void(FAssetHandle)> InOpenMaterialEditor);

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

    FWorldEditorContext& mEditorContext;
    FAssetRegistry* mAssetRegistry{nullptr};
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
