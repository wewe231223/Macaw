#include "pch.h"
#include "Application/FApplication.h"
#include "Core/Stat/Stat.h"
#include "Resource.h"
#include "Editor/Platform/FLoadingScreen.h"
#include "Core/Console/Console.h"
#include "Editor/Input/Messages/FMousePickRequestMessage.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

FApplication::FApplication() = default;

FApplication::~FApplication() {
    Shutdown();
}

int FApplication::Run(HINSTANCE Instance, int ShowCommand) {
    mInstance = Instance;
    mShowCommand = ShowCommand;

    return mEngineLoop.Run(*this, mPlatform);
}

bool FApplication::Initialize() {
    const HACCEL AcceleratorTable{LoadAccelerators(mInstance, MAKEINTRESOURCE(IDC_MACAW))};

    if (!mPlatform.Initialize(mInstance, mShowCommand, *this, LoadIcon(mInstance, MAKEINTRESOURCE(IDI_MACAW)), LoadIcon(mInstance, MAKEINTRESOURCE(IDI_SMALL)), AcceleratorTable, static_cast<int>(mLoadingWindowWidth), static_cast<int>(mLoadingWindowHeight))) {
        return false;
    }

    mContext.mRenderer.Create(mPlatform.GetWindowHandle(), mLoadingWindowWidth, mLoadingWindowHeight);

    if (mContext.mRenderer.GetDevice() == nullptr || mContext.mRenderer.GetDeviceContext() == nullptr) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    mWindowState.mImGuiInitialized = true;

    if (!ImGui_ImplWin32_Init(static_cast<void*>(mPlatform.GetWindowHandle()))) {
        return false;
    }

    mWindowState.mImGuiPlatformInitialized = true;

    if (!ImGui_ImplDX11_Init(mContext.mRenderer.GetDevice(), mContext.mRenderer.GetDeviceContext())) {
        return false;
    }

    mWindowState.mImGuiRendererInitialized = true;
    ImGui::StyleColorsDark();

    ImGuiIO& Io{ImGui::GetIO()};

    Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    Io.Fonts->AddFontFromFileTTF("./Content/Font/NotoSansKR-Medium.ttf", 16.0f, nullptr, Io.Fonts->GetGlyphRangesKorean());

    FLoadingScreen LoadingScreen{};
    const HWND WindowHandle{mPlatform.GetWindowHandle()};
    const bool Loaded{LoadingScreen.Run(mContext.mRenderer, mPlatform, [this, WindowHandle](FLoadingProgress& Progress) {
        return InitializeApplication(Progress, WindowHandle);
    }, [this](FLoadingProgress& Progress) {
        mContext.mThumbnailRenderer->Tick();

        const float ThumbnailProgress{mContext.mThumbnailRenderer->GetGenerationProgress()};
        const bool Finished{ThumbnailProgress >= 1.0f};

        Progress.SetProgress(0.96f + ThumbnailProgress * 0.04f, Finished ? "Ready" : "Generating thumbnails");

        return Finished;
    })};

    mEditorLogo = LoadingScreen.TakeLogoShaderResourceView();

    if (!Loaded) {
        return false;
    }

    RestoreGameWindow();
    mAcceptGameInput.store(true, std::memory_order_release);
    Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Macaw Engine Initialized.");
    Stat::ResetFrameStats();
    mWindowState.mFrameEnabled = true;
    mInitialized = true;

    return true;
}

bool FApplication::InitializeApplication(FLoadingProgress& Progress, HWND WindowHandle) {
    Progress.SetProgress(0.02f, "Registering object types");
    mContext.mEngine.Initialize();

    Progress.SetProgress(0.06f, "Initializing renderer resources");

    if (!mContext.mRenderer.Initialize()) {
        return false;
    }

    Progress.SetProgress(0.10f, "Creating world services");
    mContext.mWorldContext = &mContext.mEngine.GetEditorWorldContext();
    mContext.mEditorContext = &mContext.mEngine.GetEditorContext();
    mContext.mThumbnailRenderer = std::make_unique<FAssetThumbnailRenderer>();
    mContext.mWorldCommandChannel = std::make_unique<FMessageChannel>(64);
    mContext.mEditorView = std::make_unique<EditorViewport>();
    mContext.mEditorUIManager = std::make_unique<FEditorUIManager>();

    Progress.SetProgress(0.13f, "Loading editor settings");
    mContext.mEditorSettings = mContext.mEditorContext->GetEditorSettings();

    const FAssetRegistry::FProgressCallback AssetProgressCallback{[&Progress](float AssetProgress, const std::string& Status) {
        Progress.SetProgress(0.15f + AssetProgress * 0.55f, Status);
    }};

    const bool AssetsInitialized{mContext.mEngine.GetAssetRegistry().Initialize(AssetProgressCallback)};

    if (!AssetsInitialized) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Some optional assets failed to load. Initialization will continue.");
    }

    if (!mContext.mRenderer.BindAssetRegistry(&mContext.mEngine.GetAssetRegistry())) {
        return false;
    }

    Progress.SetProgress(0.71f, "Preparing render resources");

    if (!mContext.mRenderer.PrepareAssetResources()) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Some asset render resources failed to initialize.");
    }

    Progress.SetProgress(0.73f, "Initializing editor channels");
    mContext.mMouseInput.InitializeWorldCommandSender(mContext.mWorldCommandChannel->GetSender());
    mContext.mKeyboardInput.InitializeWorldCommandSender(mContext.mWorldCommandChannel->GetSender());
    mContext.mWorldCommandChannel->TryBind<FMousePickRequestMessage>([this](const FMousePickRequestMessage& Message) {
        mContext.mEditorContext->HandleMousePickRequest(Message);
    });

    Progress.SetProgress(0.78f, "Initializing editor view");
    mContext.mEditorView->Initialize(mContext.mRenderer.GetDevice(), mContext.mEngine.GetAssetRegistry(), *mContext.mEditorContext);

    InitializeMode(mContext, WindowHandle);

    Progress.SetProgress(0.86f, "Loading scene");

    mContext.mEngine.LoadStartupScene();

    Progress.SetProgress(0.96f, "Finalizing assets");
    mContext.mEngine.GetAssetRegistry().Finalize();
    mContext.mThumbnailRenderer->Create(&mContext.mRenderer, &mContext.mEngine.GetAssetRegistry());
    Progress.SetProgress(0.96f, "Generating thumbnails");

    return true;
}

void FApplication::ProcessInput(FApplicationContext&, float) {
}

void FApplication::Tick(float DeltaTime) {
    if (!mWindowState.mFrameEnabled) {
        return;
    }

    Stat::BeginFrame();
    {
        {
            const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::FrameSetup};

            Stat::RecordObjectCounts(UObjectSystem::GetObjectCount(), mContext.mWorldContext->GetWorld().GetActors().size());
            mContext.mRenderer.BeginFrame();
        }

        {
            const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::PreviewRender};

            mContext.mEditorUIManager->RenderOffscreen(mContext.mRenderer, mContext.mEngine.GetAssetRegistry());
        }

        {
            const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::EditorUi};

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            DrawTitleBar();
            mContext.mEditorUIManager->Tick();

            for (const FPendingExternalFileDrop& Drop : mPendingExternalFileDrops) {
                mContext.mEditorUIManager->HandleExternalFileDrop(Drop.mFilePath, ImVec2{static_cast<float>(Drop.mScreenPosition.x), static_cast<float>(Drop.mScreenPosition.y)});
            }

            mPendingExternalFileDrops.clear();
        }

        {
            const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::WorldUpdate};

            ProcessInput(mContext, DeltaTime);
            mContext.mWorldCommandChannel->Dispatch();
            mContext.mEngine.Tick(DeltaTime);
            mContext.mEditorContext->Dispatch();
        }

        RenderMode(mContext, DeltaTime);

        {
            const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::UiRender};

            ImGui::Render();
            mContext.mRenderer.BeginUiRender();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                EnableExternalDropsForImGuiViewports();
                ImGui::RenderPlatformWindowsDefault();
            }
        }

        {
            const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::Present};

            mContext.mRenderer.EndFrame();
        }

        mContext.mMouseInput.EndFrame();
    }

    Stat::EndFrame();
}

void FApplication::SaveState() {
    mContext.mEditorSettings = mContext.mEditorContext->GetEditorSettings();

    if (FViewportHostWindow * Host{mContext.mEditorUIManager->GetViewportHostWindow()}) {
        Host->CaptureLayoutSettings(mContext.mEditorSettings);
    }

    mContext.mEngine.SaveEditorSettings(mContext.mEditorSettings);
}

void FApplication::Shutdown() {
    if (mInitialized) {
        SaveState();
        mInitialized = false;
    }

    mWindowState.mFrameEnabled = false;
    mAcceptGameInput.store(false, std::memory_order_release);

    if (mContext.mThumbnailRenderer != nullptr) {
        mContext.mThumbnailRenderer->Terminate();
    }

    if (mContext.mEditorUIManager != nullptr) {
        mContext.mEditorUIManager->ReleaseRenderResources();
    }

    if (mWindowState.mImGuiInitialized) {
        if (mWindowState.mImGuiRendererInitialized) {
            ImGui_ImplDX11_Shutdown();
            mWindowState.mImGuiRendererInitialized = false;
        }

        if (mWindowState.mImGuiPlatformInitialized) {
            ImGui_ImplWin32_Shutdown();
            mWindowState.mImGuiPlatformInitialized = false;
        }

        ImGui::DestroyContext();
        mWindowState.mImGuiInitialized = false;
    }

    mContext.mRenderer.BindAssetRegistry(nullptr);
    mContext.mMenuPanel.reset();
    mContext.mEditorUIManager.reset();
    mContext.mEditorView.reset();
    mContext.mThumbnailRenderer.reset();
    mContext.mWorldCommandChannel.reset();
    mContext.mEditorContext = nullptr;
    mContext.mWorldContext = nullptr;
    mContext.mEngine.Shutdown();
    mEditorLogo.Reset();
    mPendingExternalFileDrops.clear();
    mPlatform.Shutdown();

    if (mContext.mRenderer.GetDeviceContext() != nullptr) {
        mContext.mRenderer.Terminate();
        mContext.mRenderer.ReportLiveObjects();
    }
}
