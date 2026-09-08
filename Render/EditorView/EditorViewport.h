#pragma once 

#include <d3d11.h>
#include "../Core/Channel/FStateChannel.h"
#include "../RenderWindowInfo.h"
#include "FLineRenderer.h"
#include "../../Core/Base/FRenderProbe.h"

class EditorViewport {
	constexpr static float  OrientationAxisSize = 200.0f;
public:
	EditorViewport() = default;
	~EditorViewport() = default;

	EditorViewport(const EditorViewport&) = delete;
	EditorViewport& operator=(const EditorViewport&) = delete;

	EditorViewport(EditorViewport&&) noexcept = default;
	EditorViewport& operator=(EditorViewport&&) noexcept = default;

public:
	void Initialize(ID3D11Device* Device, TStateChannel<RenderWindowInfo>::FReader WindowReader);

	void Render(ID3D11DeviceContext* Context, CameraProbe& Probe);
private:
	void RenderGrid(ELineDepthMode DepthMode);
	void RenderAxis(ELineDepthMode DepthMode); 
	void RenderOrientationAxis(ID3D11DeviceContext* Context, CameraProbe& Probe); 

private:
	TStateChannel<RenderWindowInfo>::FReader WindowInfoReader{};
	FLineRenderer LineRenderer{};

	D3D11_VIEWPORT OrientationAxisViewport{ 5.0f, 5.0f, OrientationAxisSize, OrientationAxisSize, 0.0f, 1.0f };
};