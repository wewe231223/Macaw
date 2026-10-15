/**
 * @file FMaterialEditorPanel.h
 * @brief 전역 에셋을 편집하며 패널별 프리뷰를 보관하는 머티리얼 편집기를 정의합니다.
 */
#pragma once
#include "Editor/Panel/FEditorWindow.h"
#include "Editor/Panel/FPropertyEditorContext.h"
#include "Render/FOffScreenRenderSurface.h"
#include "Core/Base/FAssetHandle.h"
#include "Asset/FMaterialGroup.h"
#include <functional>
#include "Editor/View/FAssetThumbnailRenderer.h"
#include "Render/Renderer.h"
#include "Asset/USurfaceOpaque.h"

class FMaterialEditorPanel final : public FEditorWindow {
public:
    /**
     * @brief 전역 에셋 서비스를 사용하는 머티리얼 편집 패널을 생성합니다.
     * @param InThumbnailRenderer 프리뷰와 썸네일을 생성할 렌더러입니다.
     * @pre 패널 사용 중에는 활성 엔진의 에셋 서비스가 존재해야 합니다.
     */
    FMaterialEditorPanel(FAssetThumbnailRenderer& InThumbnailRenderer);
    ~FMaterialEditorPanel() override;

    FMaterialEditorPanel(const FMaterialEditorPanel&) = delete;
    FMaterialEditorPanel& operator=(const FMaterialEditorPanel&) = delete;
    FMaterialEditorPanel(FMaterialEditorPanel&&) = delete;
    FMaterialEditorPanel& operator=(FMaterialEditorPanel&&) = delete;

public:
    void OpenMaterial(FAssetHandle MaterialHandle);
    /**
     * @brief 전역 레지스트리의 머티리얼로 이 패널의 프리뷰를 갱신합니다.
     * @param Renderer 프리뷰 표면에 사용할 렌더러입니다.
     */
    void RenderOffscreen(FRenderer& Renderer) override;
    void ReleaseRenderResources() override;

private:
    void DrawContents() override;
    void DrawGroup(USurfaceOpaque& Material, Uint32 GroupIndex, const FMaterialGroup& Group);
    void ModifyGroup(USurfaceOpaque& Material, Uint32 GroupIndex, const std::function<void(FMaterialGroup&)>& Modifier);

private:
    FAssetThumbnailRenderer& mThumbnailRenderer;
    FPropertyEditorContext mPropertyEditor{};
    FOffScreenRenderSurface mPreviewSurface{};
    FAssetHandle mMaterialHandle{};
    bool mPreviewDirty{false};
};
