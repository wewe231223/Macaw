#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Base/FScopedFlag.h"
#include "Editor/UEditorEngine.h"
#include "Editor/Settings/FEditorConfigManager.h"
#include "Editor/World/FWorldEditorContext.h"
#include "Editor/UndoSystem/FUndoSystem.h"
#include "Engine/Serialization/FSceneSerializer.h"
#include "Engine/Serialization/FWorldCloner.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Core/Console/Console.h"
#include "World/UWorld.h"
#include "Editor/Panel/FEditorUIManager.h"
#include "Editor/View/FAssetThumbnailRenderer.h"
#include "Editor/Platform/FLoadingScreen.h"
#include "Editor/Input/Messages/FMousePickRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraRotateRequestMessage.h"
#include "Editor/Input/Messages/FKeyboardCameraMoveRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraMoveRequestMessage.h"
#include "Editor/Input/Messages/FMouseCameraDollyRequestMessage.h"
#include "Asset/FAssetRegistry.h"
#include "Core/Stat/Stat.h"
#include "Render/Renderer.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

namespace {
    UEditorEngine* EditorEngineInstance{};
}

UEditorEngine* GetEditorEngine() noexcept {
    return EditorEngineInstance == GetEngine() ? EditorEngineInstance : nullptr;
}

UEditorEngine::UEditorEngine() {
    InitializeClass(StaticClass());
}

UEditorEngine::~UEditorEngine() {
    Shutdown();
}

void UEditorEngine::Initialize() {
    if (mEditorContext != nullptr) {
        return;
    }

    Super::Initialize();

    if (!IsInitialized()) {
        return;
    }

    ReflectionModules::RegisterEditor();
    mEditorWorldContext = &CreateWorldContext(EWorldType::Editor);
    mEditorContext = std::make_unique<FWorldEditorContext>();

    FEditorConfigManager& Config{FEditorConfigManager::Get()};

    Config.GetSettings() = {};
    Config.Reload();
    mEditorContext->SetWorld(&mEditorWorldContext->GetWorld());
    mEditorContext->InitializeChannels();
    FUndoSystem::Reset();
    EditorEngineInstance = this;
}

void UEditorEngine::Shutdown() {
    if (!IsInitialized() && mEditorContext == nullptr && GetRenderer() == nullptr && !mImGuiInitialized) {
        return;
    }

    if (!CanChangeWorldContexts() || mProcessingPIE) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Cannot shut down the editor engine during world callbacks.");
        return;
    }

    const FScopedFlag ProcessingPIE{mProcessingPIE};

    StopPIE();
    ReleaseEditorResources();
    FUndoSystem::Reset();

    if (EditorEngineInstance == this) {
        EditorEngineInstance = nullptr;
    }

    mEditorContext.reset();
    mEditorWorldContext = nullptr;
    mScenePath.clear();
    Super::Shutdown();
}

void UEditorEngine::Tick(float DeltaTime) {
    if (!IsInitialized() || mProcessingPIE || !CanChangeWorldContexts()) {
        return;
    }

    const FScopedFlag ProcessingPIE{mProcessingPIE};

    ProcessPIERequests();
    Super::Tick(DeltaTime);
    ProcessPIERequests();
}

bool UEditorEngine::RequestStartPIE() {
    if (!IsInitialized() || mEditorWorldContext == nullptr || mPIEState != EPIEState::Stopped) {
        return false;
    }

    mPauseRequested = false;
    mPIEState = EPIEState::Starting;

    return true;
}

bool UEditorEngine::RequestStopPIE() {
    if (mPIEState == EPIEState::Stopped || mPIEState == EPIEState::Stopping) {
        return false;
    }

    mPIEState = EPIEState::Stopping;

    return true;
}

bool UEditorEngine::RequestPausePIE() {
    if (!HasPIESession() || mPIEState == EPIEState::Stopping || mPauseRequested) {
        return false;
    }

    mPauseRequested = true;

    return true;
}

bool UEditorEngine::RequestResumePIE() {
    if (!HasPIESession() || mPIEState == EPIEState::Stopping || !mPauseRequested) {
        return false;
    }

    mPauseRequested = false;

    return true;
}

EPIEState UEditorEngine::GetPIEState() const {
    return mPIEState;
}

bool UEditorEngine::HasPIESession() const {
    return mPIEState != EPIEState::Stopped;
}

FWorldContext* UEditorEngine::GetPIEWorldContext() const {
    return mPIEWorldContext;
}

bool UEditorEngine::DestroyWorldContext(FWorldContext& Context) {
    if (&Context == mEditorWorldContext || &Context == mPIEWorldContext) {
        return false;
    }

    return Super::DestroyWorldContext(Context);
}

void UEditorEngine::ProcessPIERequests() {
    if (mPIEState == EPIEState::Starting) {
        StartPIE();
    }

    if (mPIEState == EPIEState::Stopping || (mPIEWorldContext != nullptr && !mPIEWorldContext->GetWorld().HasBegunPlay())) {
        StopPIE();
        return;
    }

    if (mPIEWorldContext != nullptr) {
        mPIEWorldContext->GetWorld().GetTime().SetPaused(mPauseRequested);
        mPIEState = mPauseRequested ? EPIEState::Paused : EPIEState::Playing;
    }
}

void UEditorEngine::StartPIE() {
    std::unique_ptr<UWorld> World{FWorldCloner::Clone(mEditorWorldContext->GetWorld(), EWorldType::PIE)};

    if (mPIEState == EPIEState::Stopping) {
        ResetPIEState();
        return;
    }

    if (World == nullptr) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to clone the editor world for PIE.");
        ResetPIEState();
        return;
    }

    mPIEWorldContext = AdoptWorld(World);

    if (mPIEWorldContext == nullptr) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to create a PIE world context.");
        ResetPIEState();
        return;
    }

    mPIEWorldContext->GetWorld().BeginPlay();
}

void UEditorEngine::StopPIE() {
    if (mPIEWorldContext != nullptr) {
        mPIEState = EPIEState::Stopping;

        mPIEWorldContext->GetWorld().EndPlay(EEndPlayReason::EndPlayInEditor);

        if (!Super::DestroyWorldContext(*mPIEWorldContext)) {
            return;
        }
    }

    ResetPIEState();
}

void UEditorEngine::ResetPIEState() {
    mPIEWorldContext = nullptr;
    mPIEState = EPIEState::Stopped;
    mPauseRequested = false;
}

FWorldContext& UEditorEngine::GetEditorWorldContext() {
    if (mEditorWorldContext == nullptr) {
        ErrorHandler::Report("UEditorEngine", "Editor engine is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    return *mEditorWorldContext;
}

FWorldEditorContext& UEditorEngine::GetEditorContext() {
    if (mEditorContext == nullptr) {
        ErrorHandler::Report("UEditorEngine", "Editor engine is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    return *mEditorContext;
}

bool UEditorEngine::LoadStartupScene() {
    if (mEditorContext == nullptr) {
        return false;
    }

    const FString LastScenePath{FEditorConfigManager::Get().GetSettings().mLastLoadedScenePath};
    const std::filesystem::path ScenePath{LastScenePath.empty() ? "scenes/Default.json" : LastScenePath.c_str()};

    return LoadSceneInternal(ScenePath, !LastScenePath.empty());
}

bool UEditorEngine::LoadScene(const std::filesystem::path& ScenePath) {
    return LoadSceneInternal(ScenePath, true);
}

bool UEditorEngine::LoadSceneInternal(const std::filesystem::path& ScenePath, bool BRememberScene) {
    if (mEditorWorldContext == nullptr || mEditorContext == nullptr || HasPIESession()) {
        return false;
    }

    if (!FSceneSerializer::Load(mEditorWorldContext->GetWorld(), ScenePath)) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to load scene: %s", ScenePath.generic_string().c_str());
        return false;
    }

    mEditorContext->ClearSelection();
    FUndoSystem::Reset();

    if (BRememberScene) {
        SetScenePath(ScenePath);
    } else {
        mScenePath.clear();
    }

    Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Loaded scene: %s", ScenePath.generic_string().c_str());

    return true;
}

bool UEditorEngine::SaveScene(const FString& SceneName) {
    if (mEditorWorldContext == nullptr || mEditorContext == nullptr) {
        return false;
    }

    const std::filesystem::path Name{SceneName.c_str()};

    if (Name.empty() || Name.has_parent_path() || Name == "." || Name == ".." || SceneName.find_first_of("<>:\"/\\|?*") != FString::npos) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Invalid scene name: %s", SceneName.c_str());
        return false;
    }

    std::filesystem::path ScenePath{"scenes"};

    ScenePath /= Name;

    if (ScenePath.extension() != ".json") {
        ScenePath += ".json";
    }

    if (!mScenePath.empty() && mScenePath.extension() == ".json" && mScenePath.stem() == ScenePath.stem()) {
        ScenePath = mScenePath;
    }

    if (!FSceneSerializer::Save(mEditorWorldContext->GetWorld(), ScenePath)) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to save scene: %s", ScenePath.generic_string().c_str());
        return false;
    }

    SetScenePath(ScenePath);
    Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Saved scene: %s", mScenePath.generic_string().c_str());

    return true;
}

bool UEditorEngine::SaveEditorSettings() {
    if (mEditorContext == nullptr) {
        return false;
    }

    FEditorConfigManager::Get().GetSettings().mLastLoadedScenePath = mScenePath.generic_string().c_str();

    if (!FEditorConfigManager::Get().Flush()) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Failed to save editor settings.");
        return false;
    }

    return true;
}

const std::filesystem::path& UEditorEngine::GetScenePath() const {
    return mScenePath;
}

void UEditorEngine::SetScenePath(const std::filesystem::path& ScenePath) {
    mScenePath = ScenePath.lexically_normal();

    SaveEditorSettings();
}

bool UEditorEngine::InitializeEditor(HWND WindowHandle, UINT Width, UINT Height, IPlatformApplication& Platform, EEditorMode Mode) {
    if (IsInitialized() || ImGui::GetCurrentContext() != nullptr || !CreateRenderer(WindowHandle, Width, Height)) {
        return false;
    }

    FRenderer& Renderer{*GetRenderer()};

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    mImGuiInitialized = true;
    mImGuiPlatformInitialized = ImGui_ImplWin32_Init(static_cast<void*>(WindowHandle));

    if (!mImGuiPlatformInitialized) {
        Shutdown();
        return false;
    }

    mImGuiRendererInitialized = ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());

    if (!mImGuiRendererInitialized) {
        Shutdown();
        return false;
    }

    ImGui::StyleColorsDark();

    ImGuiIO& Io{ImGui::GetIO()};

    Io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    Io.Fonts->AddFontFromFileTTF("./Content/Font/NotoSansKR-Medium.ttf", 16.0f, nullptr, Io.Fonts->GetGlyphRangesKorean());

    FLoadingScreen LoadingScreen{};
    const bool Loaded{LoadingScreen.Run(Renderer, Platform, [this, WindowHandle, Mode](FLoadingProgress& Progress) {
        return InitializeEditorResources(Progress, WindowHandle, Mode);
    }, [this](FLoadingProgress& Progress) {
        mThumbnailRenderer->Tick();

        const float ThumbnailProgress{mThumbnailRenderer->GetGenerationProgress()};
        const bool Finished{ThumbnailProgress >= 1.0f};

        Progress.SetProgress(0.96f + ThumbnailProgress * 0.04f, Finished ? "Ready" : "Generating thumbnails");

        return Finished;
    })};

    mLogo = LoadingScreen.TakeLogoShaderResourceView();

    if (!Loaded) {
        Shutdown();
        return false;
    }

    Stat::ResetFrameStats();
    mPresentationReady = true;

    return true;
}

bool UEditorEngine::InitializeEditorResources(FLoadingProgress& Progress, HWND WindowHandle, EEditorMode Mode) {
    Progress.SetProgress(0.02f, "Registering object types");
    Initialize();

    if (!IsInitialized()) {
        return false;
    }

    Progress.SetProgress(0.06f, "Initializing renderer resources");

    FRenderer& Renderer{*GetRenderer()};

    if (!Renderer.Initialize()) {
        return false;
    }

    Progress.SetProgress(0.10f, "Creating editor resources");
    mThumbnailRenderer = std::make_unique<FAssetThumbnailRenderer>();
    mWorldCommandChannel = std::make_unique<FMessageChannel>(64);
    mEditorUI = std::make_unique<FEditorUIManager>();

    const FAssetRegistry::FProgressCallback AssetProgressCallback{[&Progress](float AssetProgress, const std::string& Status) {
        Progress.SetProgress(0.15f + AssetProgress * 0.55f, Status);
    }};

    FAssetRegistry& Assets{FAssetRegistry::Get()};

    if (!Assets.Initialize(AssetProgressCallback)) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Some optional assets failed to load. Initialization will continue.");
    }

    Progress.SetProgress(0.71f, "Preparing render resources");

    if (!Renderer.PrepareAssetResources()) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Some asset render resources failed to initialize.");
    }

    Progress.SetProgress(0.73f, "Initializing editor channels");
    mMouseInput.InitializeWorldCommandSender(mWorldCommandChannel->GetSender());
    mKeyboardInput.InitializeWorldCommandSender(mWorldCommandChannel->GetSender());
    mWorldCommandChannel->TryBind<FMousePickRequestMessage>([this](const FMousePickRequestMessage& Message) {
        mEditorContext->HandleMousePickRequest(Message);
    });

    Progress.SetProgress(0.78f, "Initializing editor view");

    if (Mode == EEditorMode::Viewer) {
#ifdef OBJ_VIEWER
        mWorldCommandChannel->TryBind<FMouseCameraRotateRequestMessage>([this](const FMouseCameraRotateRequestMessage& Message) {
            mEditorContext->HandleMouseCameraRotateRequest(Message);
        });

        mWorldCommandChannel->TryBind<FKeyboardCameraMoveRequestMessage>([this](const FKeyboardCameraMoveRequestMessage& Message) {
            mEditorContext->HandleKeyboardCameraMoveRequest(Message);
        });

        mWorldCommandChannel->TryBind<FMouseCameraMoveRequestMessage>([this](const FMouseCameraMoveRequestMessage& Message) {
            mEditorContext->HandleMouseCameraMoveRequestMessage(Message);
        });

        mWorldCommandChannel->TryBind<FMouseCameraDollyRequestMessage>([this](const FMouseCameraDollyRequestMessage& Message) {
            mEditorContext->HandleMouseCameraDollyRequestMessage(Message);
        });
#endif

        mEditorUI->InitializeViewer(WindowHandle, mThumbnailRenderer.get());
    } else {
        mEditorUI->Initialize(Renderer, WindowHandle, mThumbnailRenderer.get());
    }

    Progress.SetProgress(0.86f, "Loading scene");
    LoadStartupScene();
    Progress.SetProgress(0.96f, "Finalizing assets");
    Assets.Finalize();
    mThumbnailRenderer->Create(&Renderer);
    Progress.SetProgress(0.96f, "Generating thumbnails");

    return true;
}

void UEditorEngine::TickFrame(float DeltaTime, const std::function<void()>& DrawWindowChrome) {
    if (!mPresentationReady) {
        return;
    }

    Stat::BeginFrame();

    {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::FrameSetup};

        Stat::RecordObjectCounts(UObjectSystem::GetObjectCount(), GetEditorWorldContext().GetWorld().GetActors().size());
        GetRenderer()->BeginFrame();
    }

    {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::PreviewRender};

        mEditorUI->RenderOffscreen(*GetRenderer());
    }

    DrawEditorUI(DrawWindowChrome);

    {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::WorldUpdate};

        mEditorUI->ProcessInput(mKeyboardInput, mMouseInput, DeltaTime);
        mWorldCommandChannel->Dispatch();
        Tick(DeltaTime);
        mEditorContext->Dispatch();
    }

    {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::RenderPreparation};

        const FRenderScene* RenderScene{mEditorUI->UpdateRenderData(*GetRenderer())};

        mEditorUI->RenderViews(*GetRenderer(), RenderScene);
    }

    RenderUI();

    {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::Present};

        GetRenderer()->EndFrame();
    }

    mMouseInput.EndFrame();
    Stat::EndFrame();
}

void UEditorEngine::DrawEditorUI(const std::function<void()>& DrawWindowChrome) {
    const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::EditorUi};

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (DrawWindowChrome) {
        DrawWindowChrome();
    }

    mEditorUI->Tick();

    for (const FExternalFileDrop& Drop : mExternalFileDrops) {
        mEditorUI->HandleExternalFileDrop(Drop.mFilePath, ImVec2{static_cast<float>(Drop.mScreenPosition.x), static_cast<float>(Drop.mScreenPosition.y)});
    }

    mExternalFileDrops.clear();
}

void UEditorEngine::RenderUI() {
    const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::UiRender};

    ImGui::Render();
    GetRenderer()->BeginUiRender();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void UEditorEngine::ProcessWindowInput(UINT Message, WPARAM WParam, LPARAM LParam) {
    if (mPresentationReady) {
        mMouseInput.ProcessWindowMessage(Message, WParam, LParam);
        mKeyboardInput.ProcessWindowMessage(Message, WParam, LParam);
    }
}

void UEditorEngine::QueueExternalFileDrop(const std::filesystem::path& FilePath, POINT ScreenPosition) {
    if (mPresentationReady) {
        mExternalFileDrops.push_back({FilePath, ScreenPosition});
    }
}

void UEditorEngine::DrawMainMenu() {
    if (mEditorUI != nullptr) {
        mEditorUI->DrawMainMenu();
    }
}

ID3D11ShaderResourceView* UEditorEngine::GetLogo() const {
    return mLogo.Get();
}

void UEditorEngine::ReleaseEditorResources() {
    if (mPresentationReady) {
        if (FViewportHostWindow* Host{mEditorUI->GetViewportHostWindow()}) {
            Host->CaptureLayoutSettings();
        }

        SaveEditorSettings();
        mPresentationReady = false;
    }

    if (mEditorUI != nullptr) {
        mEditorUI->ReleaseRenderResources();
        mEditorUI.reset();
    }

    if (mThumbnailRenderer != nullptr) {
        mThumbnailRenderer->Terminate();
        mThumbnailRenderer.reset();
    }

    mKeyboardInput = {};
    mMouseInput = {};
    mWorldCommandChannel.reset();
    mExternalFileDrops.clear();
    mLogo.Reset();

    if (mImGuiRendererInitialized) {
        ImGui_ImplDX11_Shutdown();
        mImGuiRendererInitialized = false;
    }

    if (mImGuiPlatformInitialized) {
        ImGui_ImplWin32_Shutdown();
        mImGuiPlatformInitialized = false;
    }

    if (mImGuiInitialized) {
        ImGui::DestroyContext();
        mImGuiInitialized = false;
    }
}
