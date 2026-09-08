#include "PCH.h"
#include "EditorViewport.h"

#include <ranges>

void EditorViewport::Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry, TStateChannel<RenderWindowInfo>::FReader windowReader, TStateChannel<TObjectRef<UCollisionComponent>>::FReader selectedActorReader) {
	LineRenderer.Initialize(Device);
	TransformGizmo.Initialize(Device, AssetRegistry);
	WindowInfoReader = windowReader;
	SelectedActorReader = selectedActorReader;
}

void EditorViewport::RenderInProbe(FRenderProbe& Probe) {
	if (SelectedActorReader.HasValue()) {
		if (SelectedActorReader.Peek().Get() != nullptr) {
			if (SelectedActorReader.HasChanged()) {
				FMatrix TargetTransform = SelectedActorReader.Read().Get()->GetWorldMatrix();
				TransformGizmo.SetGizmoWorldTransform(TargetTransform, SelectedActorReader.Read().Get()->GetExtent());
			}
		 
			TransformGizmo.Render(Probe);
		}
		else {
			// 비활성화 
		}

	}
}

void EditorViewport::Render(ID3D11DeviceContext* Context, FRenderProbe& Probe) {
	ELineDepthMode DepthMode = ELineDepthMode::DepthTested;

	RenderGrid(DepthMode);
	RenderAxis(DepthMode); 
	
	LineRenderer.Render(Context, FLineViewData{
		.ViewProjection = Probe.MainCameraProbe.ViewProjection,
		.ViewportSize = FVector2D { WindowInfoReader.Read().Viewport.Width, WindowInfoReader.Read().Viewport.Height }
	});

	RenderOrientationAxis(Context, Probe.MainCameraProbe);
}

void EditorViewport::RenderGrid(ELineDepthMode DepthMode) {
	constexpr int GridSize = 50;
	constexpr float LineLength = static_cast<float>(GridSize); 

	for (auto x : std::views::iota(-GridSize, GridSize + 1)) {
		if (x == 0) continue; 
		LineRenderer.AddLine(FVector3{ static_cast<float>(x), 0.f, -LineLength }, FVector3{ static_cast<float>(x), 0.f, LineLength }, FVector4{ 0.5f, 0.5f, 0.5f, 1.0f }, 1.0f, DepthMode);
	}

	for (auto z : std::views::iota(-GridSize, GridSize + 1)) {
		if (z == 0) continue;
		LineRenderer.AddLine(FVector3{ -LineLength, 0.f, static_cast<float>(z) }, FVector3{ LineLength, 0.f, static_cast<float>(z) }, FVector4{ 0.5f, 0.5f, 0.5f, 1.0f }, 1.0f, DepthMode);
	}
}

void EditorViewport::RenderAxis(ELineDepthMode DepthMode) {
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 1.0f, 0.0f, 0.0f }, 1000.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ -1.0f, 0.0f, 0.0f }, 1000.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, DepthMode);
																																	   
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 1.0f, 0.0f }, 1000.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, -1.0f, 0.0f }, 1000.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, DepthMode);
																																	   
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, 1.0f }, 1000.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, DepthMode);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, -1.0f }, 1000.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, DepthMode);
}

void EditorViewport::RenderOrientationAxis(ID3D11DeviceContext* Context, CameraProbe& Probe) {
	FMatrix view = Probe.View;
	view.Translation(FVector3{ 0.0f, 0.0f, 3.0f });

	FMatrix proj = FMatrix::CreateOrthographic(2.5f, 2.5f, 0.1f, 10.f);

	Context->RSSetViewports(1, &OrientationAxisViewport);

	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 1.0f, 0.0f, 0.0f }, 1.0f, FVector4{ 1.0f, 0.0f, 0.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 1.0f, 0.0f }, 1.0f, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);
	LineRenderer.AddRay(FVector3{ 0.0f, 0.0f, 0.0f }, FVector3{ 0.0f, 0.0f, 1.0f }, 1.0f, FVector4{ 0.0f, 0.0f, 1.0f, 1.0f }, 3.0f, ELineDepthMode::Overlay);
	
	
	LineRenderer.Render(Context, FLineViewData{
		.ViewProjection = view * proj,
		.ViewportSize = FVector2D{ OrientationAxisViewport.Width, OrientationAxisViewport.Height }
		}
	);
}

