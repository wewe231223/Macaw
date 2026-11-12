#pragma once

#include "Core/Types.h"
#include "FAnimSequenceTimeline.h"
#include "Editor/FAssetPicker.h"

#include <array>
#include <filesystem>
#include <memory>

namespace Asset {
    class FAssetManager;
    class UAnimSequence;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Editor {

    class FAnimationPreviewScene;
    class FAnimSequenceViewportClient;
    class FAssetThumbnailCache;
    class FEditorAssetCatalog;

    class FAnimSequenceEditor {
    public:
        FAnimSequenceEditor(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets, Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails);
        ~FAnimSequenceEditor();

    public:
        bool Open(const std::filesystem::path& Path, Core::FString& Error);
        bool SetPreviewMesh(const Core::FString& MeshPath, const Core::FString& SkeletonPath);

        void Draw(float DeltaTime, bool CanSave);
        void CloseRetired(const std::filesystem::path& Path);

        bool IsOpen() const;
        bool IsFocused() const;

    private:
        bool OpenNow(const std::filesystem::path& Path, Core::FString& Error);
        bool Save();
        void Close();
        void RefreshAssets();

        void DrawSettings(bool CanSave);
        void DrawAssetBrowser();
        void DrawClosePrompt(bool CanSave);

        void HandleShortcuts(bool CanSave);
        void StepFrame(Core::Int32 Direction);

    private:
        RenderCore::IRendererModule* mRenderer{};
        Asset::FAssetManager* mAssets{};
        Editor::FEditorAssetCatalog* mCatalog{};
        Editor::FAssetThumbnailCache* mThumbnails{};
        Editor::FAssetPicker mPicker{};
        Editor::FAnimSequenceTimeline mTimeline{};
        const Asset::UAnimSequence* mSource{};
        std::unique_ptr<Asset::UAnimSequence> mEditing{};
        std::unique_ptr<Editor::FAnimationPreviewScene> mPreview{};
        std::unique_ptr<Editor::FAnimSequenceViewportClient> mViewport{};
        std::filesystem::path mPath{};
        std::filesystem::path mPendingPath{};
        Core::FString mMeshPath{};
        Core::FString mSkeletonPath{};
        Core::FString mError{};
        std::array<char, 1024> mRootBone{};
        std::array<char, 256> mAnimationSearch{};
        bool mOpen{};
        bool mDirty{};
        bool mSourceChanged{};
        bool mConfirmClose{};
        bool mShowBones{};
        bool mShowGrid{true};
        bool mWireframe{};
    };

}
