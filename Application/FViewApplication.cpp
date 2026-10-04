#include "pch.h"
#include "Application/FViewApplication.h"
#include "Editor/Input/Messages/FMouseCameraRotateRequestMessage.h"
#include "Editor/Input/Messages/FKeyboardCameraMoveRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraMoveRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraDollyRequestMessage.h"
#include "Editor/Panel/FViewerToolBar.h"

FViewApplication::FViewApplication() = default;

FViewApplication::~FViewApplication() = default;

void FViewApplication::InitializeMode(FApplicationContext& Context, HWND WindowHandle) {
    Context.mWorldCommandChannel->TryBind<FMouseCameraRotateRequestMessage>([&Context](const FMouseCameraRotateRequestMessage& Message) {
        Context.mEditorContext->HandleMouseCameraRotateRequest(Message);
    });

    Context.mWorldCommandChannel->TryBind<FKeyboardCameraMoveRequestMessage>([&Context](const FKeyboardCameraMoveRequestMessage& Message) {
        Context.mEditorContext->HandleKeyboardCameraMoveRequest(Message);
    });

    Context.mWorldCommandChannel->TryBind<FMouseCameraMoveRequestMessage>([&Context](const FMouseCameraMoveRequestMessage& Message) {
        Context.mEditorContext->HandleMouseCameraMoveRequestMessage(Message);
    });

    Context.mWorldCommandChannel->TryBind<FMouseCameraDollyRequestMessage>([&Context](const FMouseCameraDollyRequestMessage& Message) {
        Context.mEditorContext->HandleMouseCameraDollyRequestMessage(Message);
    });

    Context.mMenuPanel = std::make_unique<FViewerToolBar>(*Context.mEditorContext);
    Context.mEditorUIManager->InitializeViewer(Context.mEngine.GetAssetRegistry(), WindowHandle, *Context.mEditorContext, Context.mThumbnailRenderer.get());
}

void FViewApplication::RenderMode(FApplicationContext&, float) {
}
