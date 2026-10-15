/**
 * @file FViewerPanel.h
 * @brief 전역 에셋을 조회하며 독립된 프리뷰 월드와 카메라를 유지하는 뷰어를 정의합니다.
 */
#pragma once
#include "Core/CoreMinimal.h"
#include "Editor/Panel/FEditorWindow.h"
#include "Editor/Panel/FPropertyEditorContext.h"
#include "Render/FOffScreenRenderSurface.h"
#include "RenderCore/FRenderData.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Base/FAssetHandle.h"
#include "Editor/World/FWorldEditorContext.h"
#include "Render/Renderer.h"
#include "Editor/View/FAssetThumbnailRenderer.h"

class FViewerPanel final : public FEditorWindow {
public:
    /**
     * @brief 전역 에셋 서비스를 사용하는 뷰어를 생성합니다.
     * @param InputWindowHandle 입력과 파일 선택에 사용할 창입니다.
     * @param InEditorToWorldSender 대상 월드에 편집 요청을 전달할 송신자입니다.
     * @param InEditorContext 대상 월드의 편집 컨텍스트입니다.
     * @param InThumbnailRenderer 선택적인 썸네일 생성기입니다.
     * @pre 패널 사용 중에는 활성 엔진의 에셋 서비스가 존재해야 합니다.
     */
    FViewerPanel(HWND InputWindowHandle, FMessageChannel::FSender InEditorToWorldSender, FWorldEditorContext& InEditorContext, FAssetThumbnailRenderer* InThumbnailRenderer);
    ~FViewerPanel() override = default;

    FViewerPanel(const FViewerPanel&) = delete;
    FViewerPanel& operator=(const FViewerPanel&) = delete;
    FViewerPanel(FViewerPanel&&) = delete;
    FViewerPanel& operator=(FViewerPanel&&) = delete;

public:
    /**
     * @brief 전역 에셋 서비스로 메시를 조회해 뷰어의 독립된 프리뷰 월드를 렌더링합니다.
     * @param InRenderer 사용할 렌더러입니다.
     */
    void RenderOffscreen(FRenderer& InRenderer) override;
    void ReleaseRenderResources() override;

    void SetMesh(FAssetHandle InMeshHandle);
    void SetMaterial(FAssetHandle InMaterialHandle);
    bool HandleExternalFileDrop(const std::filesystem::path& FilePath, const ImVec2& ScreenPosition);

private:
    void DrawContents() override;
    void DrawMenuBar();
    void DrawProperties();
    void DrawPreview();

    void ResizeSurfaceIfNeeded(ID3D11Device* Device, Uint32 Width, Uint32 Height);
    void UpdatePreviewScene();
    FViewMatrices BuildPreviewCamera() const;

    void ProcessInput();
    FMatrix MakeCameraWorldMatrix(const FVector3& Eye) const;
    bool OpenViewerFile(const std::filesystem::path& FilePath);

private:
    FMessageChannel::FSender mEditorToWorldSender;
    HWND mWindowHandle{nullptr};
    FWorldEditorContext& mEditorContext;
    FPropertyEditorContext mPropertyEditor{};

    FOffScreenRenderSurface mSurface{};
    FPreviewScene mPreviewScene{};
    FAssetHandle mMeshHandle{};
    FAssetHandle mMaterialHandle{};

    Uint32 mSurfaceWidth{};
    Uint32 mSurfaceHeight{};
    Uint32 mDesiredWidth{};
    Uint32 mDesiredHeight{};

    float mDistance{5.0f};
    FVector3 mTarget{0.0f, 0.0f, 0.0f};
    float mFieldOfView{1.0472f};
    FQuat mOrbitRotation{};

    ImVec2 mDropTargetMin{};
    ImVec2 mDropTargetMax{};
};
