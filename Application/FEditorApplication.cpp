#include "pch.h"
#include "Application/FEditorApplication.h"
#include "Core/Stat/Stat.h"
#include "Editor/Panel/FControlPanel.h"
#include "Editor/View/FEditorViewport.h"

FEditorApplication::FEditorApplication() = default;

FEditorApplication::~FEditorApplication() = default;

void FEditorApplication::InitializeMode(FApplicationContext& Context, HWND WindowHandle) {
    if (Context.mEditorContext->GetEditorSettings().mControlPanelEnabled) {
        Context.mMenuPanel = std::make_unique<FControlPanel>(*Context.mEditorContext, WindowHandle, Context.mEditorContext->GetEditorToWorldSender());
    }

    Context.mEditorUIManager->Initialize(Context.mWorldContext->GetWorld(), Context.mRenderer, Context.mEngine.GetAssetRegistry(), *Context.mEditorContext, WindowHandle, Context.mEditorView->GetGizmoMode(), Context.mEditorView->GetGizmoCoordinateSpace(), Context.mThumbnailRenderer.get());
}

void FEditorApplication::ProcessInput(FApplicationContext& Context, float DeltaTime) {
    if (FViewportHostWindow * Host{Context.mEditorUIManager->GetViewportHostWindow()}) {
        Host->ProcessInput(*Context.mEditorView, Context.mKeyboardInput, Context.mMouseInput, DeltaTime);
    }
}

void FEditorApplication::RenderMode(FApplicationContext& Context, float DeltaTime) {
    FViewportHostWindow* ViewportHostWindow{Context.mEditorUIManager->GetViewportHostWindow()};

    if (ViewportHostWindow == nullptr) {
        return;
    }

    const Stat::FScopedSystemStatTimer RenderStat{Stat::ESystemStatStage::RenderPreparation};
    const AActor* SelectedActor{Context.mEditorContext->GetSelectedActor()};
    const FObjectHandle SelectedActorHandle{SelectedActor != nullptr ? SelectedActor->GetHandle() : FObjectHandle{}};

    {
        const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::SceneData};

        Context.mWorldContext->GetWorld().BuildSceneRenderData(mSceneData);
        Context.mWorldContext->GetWorld().BuildOverlayRenderData(mOverlayData, SelectedActorHandle);
    }

    const FRenderScene& RenderScene{Context.mRenderer.SynchronizeScene(mSceneData)};

    for (FViewportId Id{}; Id < FViewportHostWindow::MaximumViewportCount; ++Id) {
        FEditorViewport* Viewport{ViewportHostWindow->PrepareViewportForRender(Id)};

        if (Viewport == nullptr) {
            continue;
        }

        CameraProbe Camera{};

        if (!Viewport->BuildCameraProbe(Camera)) {
            continue;
        }

        FRenderView View{};
        FOverlayRenderData Overlay{mOverlayData};
        {
            const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::ViewSetup};

            View.mTarget = &Viewport->GetRenderSurface();
            View.mCamera = Camera;
            View.mSettings = Viewport->GetRenderSettings();
            View.mRenderMode = static_cast<ERenderMode>(Context.mEditorContext->GetRenderModeState());
            Context.mEditorView->BuildOverlayRenderData(Overlay, Camera, Viewport->GetCameraPosition(), Viewport->GetRenderViewport());
        }

        Context.mRenderer.RenderView(View, RenderScene, Overlay);
    }
}
