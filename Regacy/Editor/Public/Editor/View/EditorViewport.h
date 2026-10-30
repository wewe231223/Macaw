/**
 * @file EditorViewport.h
 * @brief 월드별 편집 상태와 전역 에셋 및 설정을 사용하는 뷰포트 도구를 정의합니다.
 */

#pragma once

#include <d3d11.h>
#include "Render/FRenderView.h"
#include "Editor/World/FWorldEditorContext.h"
#include "Editor/Input/FMouseInput.h"
#include "RenderCore/FLineRenderData.h"
#include "RenderCore/FOverlayRenderData.h"
#include "Editor/View/FTransformGizmo.h"

class EditorViewport {
public:
    EditorViewport() = default;
    ~EditorViewport() = default;

    EditorViewport(const EditorViewport&) = delete;

    EditorViewport& operator=(const EditorViewport&) = delete;

    EditorViewport(EditorViewport&&) noexcept = default;

    EditorViewport& operator=(EditorViewport&&) noexcept = default;

public:
    /**
     * @brief 대상 월드의 편집 컨텍스트에 연결하고 에셋과 설정은 전역 서비스에서 조회합니다.
     * @param Device 렌더링에 사용할 그래픽 장치입니다.
     * @param InEditorContext 선택과 조작 상태를 보관하는 월드별 컨텍스트입니다.
     */
    void Initialize(ID3D11Device* Device, FWorldEditorContext& InEditorContext);

    void PrepareInput(const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport);
    void ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, bool BMouseCapturedByUi, bool BKeyboardCapturedByUi);
    void BuildOverlayRenderData(FOverlayRenderData& Overlay, const FViewMatrices& Camera, const FVector3& CameraPosition, const D3D11_VIEWPORT& Viewport);

private:
    void BuildGrid(FLineRenderData& Lines, const FViewMatrices& Camera, const FVector3& CameraPosition, const D3D11_VIEWPORT& Viewport, FVector2D& FadeCenter, ELineDepthMode DepthMode);
    void BuildAxis(FLineRenderData& Lines, ELineDepthMode DepthMode);
    void BuildBounds(FLineRenderData& Lines, const FViewMatrices& Camera, ELineDepthMode DepthMode);
    void BuildSelectionNameTag(FOverlayRenderData& Overlay);

private:
    FTransformGizmo mTransformGizmo{};

    FWorldEditorContext* mEditorContext{nullptr};
};
