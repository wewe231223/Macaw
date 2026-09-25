#pragma once

#include <d3d11.h>

#include "../../Core/Asset/FAssetRegistry.h"
#include "../../Core/Base/FRenderProbe.h"
#include "../../Core/Channel/FStateChannel.h"
#include "../../Scene/FWorldEditorContext.h"
#include "../../FMouseInput.h"

#include "ILineRenderer.h"
#include "FLineRenderer.h"
#include "FBatchLineRender.h"
#include "FTransformGizmo.h"

class EditorViewport {
public:
    EditorViewport() = default;
    ~EditorViewport() = default;

    EditorViewport(const EditorViewport&) = delete;
    EditorViewport& operator=(const EditorViewport&) = delete;

    EditorViewport(EditorViewport&&) noexcept = default;
    EditorViewport& operator=(EditorViewport&&) noexcept = default;

    void Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry, FWorldEditorContext& InEditorContext);

    void PrepareInput(const CameraProbe& Camera, const D3D11_VIEWPORT& Viewport);
    void ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, bool BMouseCapturedByUi);
    void RenderInProbe(FRenderProbe& Probe, const CameraProbe& Camera, const D3D11_VIEWPORT& Viewport);

    void RenderSceneGuides(ID3D11DeviceContext* Context, const CameraProbe& Camera, const FVector3& CameraPosition, const D3D11_VIEWPORT& Viewport);
    void RenderOrientationAxis(ID3D11DeviceContext* Context, const CameraProbe& Probe, const D3D11_VIEWPORT& Viewport);

    FStateChannel<Uint8>::FReadWriter GetGizmoMode();
    FStateChannel<Uint8>::FReadWriter GetGizmoCoordinateSpace();

private:
    void RenderGrid(const CameraProbe& Camera, const FVector3& CameraPosition, const D3D11_VIEWPORT& Viewport, FVector2D& FadeCenter, ELineDepthMode DepthMode);
    void RenderAxis(ELineDepthMode DepthMode);
    void RenderBounds(const CameraProbe& Camera, ELineDepthMode DepthMode);

private:
    std::unique_ptr<FLineRenderer> mLineRenderer{std::make_unique<FLineRenderer>()};
    FTransformGizmo mTransformGizmo{};

    FWorldEditorContext* mEditorContext{nullptr};
};
