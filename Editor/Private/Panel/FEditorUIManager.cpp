#include "pch.h"
#include "Editor/Panel/FEditorUIManager.h"
#include "Editor/Panel/FPropertyPanel.h"
#include "Editor/Panel/FConsolePanel.h"
#include "Editor/Panel/FStatPanel.h"
#include "Editor/Panel/FViewerPanel.h"
#include "Editor/Panel/FAssetBrowserPanel.h"
#include "Editor/Panel/FMaterialEditorPanel.h"
#include "Editor/Panel/Outliner.h"
#include "Editor/Panel/FViewportHostWindow.h"
#include "Editor/World/FWorldEditorContext.h"
#include "Render/Renderer.h"
#include "Core/Base/ErrorHandler.h"
#include "Editor/UEditorEngine.h"
#include "Editor/Settings/FEditorConfigManager.h"

FEditorUIManager::FEditorUIManager() = default;
FEditorUIManager::~FEditorUIManager() = default;

void FEditorUIManager::Initialize(FRenderer& Renderer, HWND WindowHandle, FAssetThumbnailRenderer* ThumbnailRenderer) {
    UEditorEngine* Engine{GetEditorEngine()};

    ErrorHandler::Report(Engine == nullptr, "FEditorUIManager", "No editor engine is active", ErrorHandler::EErrorLevel::Critical);

    FWorldEditorContext& EditorContext{Engine->GetEditorContext()};
    UWorld& World{Engine->GetEditorWorldContext().GetWorld()};
    const FEditorSettings& Settings{FEditorConfigManager::Get().GetSettings()};

    if (Settings.mViewportPanelEnabled) {
        AddViewportHostWindow(Renderer.GetDevice(), EditorContext);
    }

    if (Settings.mPropertyPanelEnabled) {
        AddWindow(std::make_unique<FPropertyPanel>(EditorContext, ThumbnailRenderer));
    }

    if (Settings.mConsolePanelEnabled) {
        AddWindow(std::make_unique<FConsolePanel>(Console::STDOutHandle, mStatDisplayFlags));
    }

    if (Settings.mStatPanelEnabled && mViewportHostWindow != nullptr) {
        mViewportHostWindow->SetStatOverlay(std::make_unique<FStatPanel>(mStatDisplayFlags));
    }

    std::unique_ptr<FMaterialEditorPanel> MaterialWindow{};

    if (Settings.mMaterialEditorPanelEnabled && ThumbnailRenderer != nullptr) {
        MaterialWindow = std::make_unique<FMaterialEditorPanel>(*ThumbnailRenderer);
        mMaterialEditorPanel = MaterialWindow.get();
    }

    if (Settings.mAssetBrowserPanelEnabled) {
        std::unique_ptr<FAssetBrowserPanel> AssetBrowserWindow{std::make_unique<FAssetBrowserPanel>(EditorContext, ThumbnailRenderer, [this](FAssetHandle Handle) {
            if (mMaterialEditorPanel != nullptr) {
                mMaterialEditorPanel->OpenMaterial(Handle);
                mFocusMaterialEditor = true;
            }
        })};

        mAssetBrowserPanel = AssetBrowserWindow.get();
        AddWindow(std::move(AssetBrowserWindow));
    }

    if (MaterialWindow != nullptr) {
        AddWindow(std::move(MaterialWindow));
    }

    if (Settings.mOutlinerPanelEnabled) {
        AddWindow(std::make_unique<FOutlinerPanel>(World, EditorContext));
    }

    AddViewerWindow(EditorContext, WindowHandle, ThumbnailRenderer);
    mPIEViewportSession = std::make_unique<FPIEViewportSession>(Renderer, mViewportHostWindow);
}

void FEditorUIManager::InitializeViewer(HWND WindowHandle, FAssetThumbnailRenderer* ThumbnailRenderer) {
    UEditorEngine* Engine{GetEditorEngine()};

    ErrorHandler::Report(Engine == nullptr, "FEditorUIManager", "No editor engine is active", ErrorHandler::EErrorLevel::Critical);
    AddViewerWindow(Engine->GetEditorContext(), WindowHandle, ThumbnailRenderer);
}

void FEditorUIManager::Tick() {
    if (mPIEViewportSession != nullptr) {
        mPIEViewportSession->BeginFrame();
    }

    mDockSpaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (mViewerWindow != nullptr && mPreviewContext != nullptr && mPreviewContext->ConsumePreviewOpenRequest()) {
        mViewerWindow->SetVisible(true);
        ImGui::SetWindowFocus(mViewerWindow->GetWindowName());
    }

    if (mViewportHostWindow != nullptr) {
        mViewportHostWindow->PrepareFrame(mDockSpaceId);
    }

    if (mMaterialEditorPanel != nullptr && !mMaterialEditorPanel->IsVisible()) {
        mMaterialEditorPanel->ReleaseRenderResources();
    }

    for (const std::unique_ptr<IEditorPanel>& Element : mElements) {
        if (Element != nullptr && Element->IsVisible()) {
            Element->DrawPanel();
        }
    }

    if (mMaterialEditorPanel != nullptr && mMaterialEditorPanel->IsVisible() && mFocusMaterialEditor) {
        ImGui::SetWindowFocus(mMaterialEditorPanel->GetWindowName());
        mFocusMaterialEditor = false;
    }

    if (mPIEViewportSession != nullptr) {
        mPIEViewportSession->DrawWindow();
    }
}

void FEditorUIManager::RenderOffscreen(FRenderer& Renderer) {

    for (const std::unique_ptr<IEditorPanel>& Element : mElements) {
        if (Element != nullptr && Element->IsVisible()) {
            Element->RenderOffscreen(Renderer);
        }
    }
}

void FEditorUIManager::ReleaseRenderResources() {
    if (mPIEViewportSession != nullptr) {
        mPIEViewportSession->ReleaseRenderResources();
    }

    for (const std::unique_ptr<IEditorPanel>& Element : mElements) {
        if (Element != nullptr) {
            Element->ReleaseRenderResources();
        }
    }
}

ImGuiID FEditorUIManager::GetDockSpaceId() const {
    return mDockSpaceId;
}

FViewportHostWindow* FEditorUIManager::GetViewportHostWindow() const {
    return mViewportHostWindow;
}

FPIEViewportSession* FEditorUIManager::GetPIEViewportSession() const {
    return mPIEViewportSession.get();
}

void FEditorUIManager::DrawPIEControls() {
    if (mPIEViewportSession != nullptr) {
        mPIEViewportSession->DrawControls();
    }
}

bool FEditorUIManager::HandleExternalFileDrop(const std::filesystem::path& FilePath, const ImVec2& ScreenPosition) {
#ifdef OBJ_VIEWER
    return mViewerWindow != nullptr && mViewerWindow->HandleExternalFileDrop(FilePath, ScreenPosition);
#else
    return mAssetBrowserPanel != nullptr && mAssetBrowserPanel->IsVisible() && mAssetBrowserPanel->HandleExternalFileDrop(FilePath, ScreenPosition);
#endif
}

void FEditorUIManager::AddWindow(std::unique_ptr<FEditorWindow> Window) {
    mElements.emplace_back(std::move(Window));
}

void FEditorUIManager::AddViewerWindow(FWorldEditorContext& EditorContext, HWND WindowHandle, FAssetThumbnailRenderer* ThumbnailRenderer) {
    if (!FEditorConfigManager::Get().GetSettings().mViewerPanelEnabled) {
        return;
    }

    std::unique_ptr<FViewerPanel> Window{std::make_unique<FViewerPanel>(WindowHandle, EditorContext.GetEditorToWorldSender(), EditorContext, ThumbnailRenderer)};

    mViewerWindow = Window.get();
    mPreviewContext = &EditorContext;
    AddWindow(std::move(Window));
}

void FEditorUIManager::AddViewportHostWindow(ID3D11Device* Device, FWorldEditorContext& EditorContext) {
    std::unique_ptr<FViewportHostWindow> Window{std::make_unique<FViewportHostWindow>(Device, EditorContext)};

    mViewportHostWindow = Window.get();
    AddWindow(std::move(Window));
}
