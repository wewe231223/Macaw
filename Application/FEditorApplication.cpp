#include "pch.h"
#include "FEditorApplication.h"

#include "Editor/Panel/FControlPanel.h"
#include "Editor/View/FEditorViewport.h"

FEditorApplication::FEditorApplication() = default;

FEditorApplication::~FEditorApplication() = default;

void FEditorApplication::InitializeMode(FApplicationContext& Context, HWND WindowHandle) {
    Context.mMenuPanel = std::make_unique<FControlPanel>(*Context.mEditorContext, WindowHandle, Context.mEditorContext->GetEditorToWorldSender());
    Context.mEditorUIManager->Initialize(*Context.mWorld, Context.mRenderer, *Context.mAssetRegistry, *Context.mEditorContext, WindowHandle, Context.mEditorView->GetGizmoMode(), Context.mEditorView->GetGizmoCoordinateSpace(), Context.mThumbnailRenderer.get());
}

void FEditorApplication::TickMode(FApplicationContext& Context, float DeltaTime) {
    FViewportHostWindow* ViewportHostWindow{Context.mEditorUIManager->GetViewportHostWindow()};
    ViewportHostWindow->ProcessInput(*Context.mEditorView, Context.mKeyboardInput, Context.mMouseInput, DeltaTime);
    Context.mWorldCommandChannel->Dispatch();
    Context.mWorld->Tick(DeltaTime);
    Context.mEditorContext->Dispatch();

    for (FViewportId Id{}; Id < FViewportHostWindow::MaximumViewportCount; ++Id) {
        FEditorViewport* Viewport{ViewportHostWindow->PrepareViewportForRender(Id)};
        if (Viewport == nullptr) {
            continue;
        }

        CameraProbe Camera{};
        if (!Viewport->BuildCameraProbe(Camera)) {
            continue;
        }

        FRenderProbe& Probe{Context.mWorld->BuildRenderProbe()};
        Context.mEditorView->RenderInProbe(Probe, Camera, Viewport->GetRenderViewport());
        Context.mRenderer.RenderScene(Viewport->GetRenderSurface(), Probe, Camera, Viewport->GetRenderSettings());
        Context.mEditorView->RenderSceneGuides(Context.mRenderer.GetDeviceContext(), Camera, Viewport->GetCameraPosition(), Viewport->GetRenderViewport());
        Context.mRenderer.RenderGizmos(Viewport->GetRenderSurface(), Probe, Camera);
        Context.mRenderer.RenderText(Probe, Camera);
        Context.mEditorView->RenderOrientationAxis(Context.mRenderer.GetDeviceContext(), Camera, Viewport->GetRenderViewport());
    }
}
