/**
 * @file FEditorUI.cpp
 * @brief 편집 상태를 소유하지 않는 ImGui 창, 패널과 뷰포트 호스트를 구현합니다.
 * @ingroup Editor
 */

#include "FEditorUI.h"
#include "Core/Stats.h"
#include "Core/FilePath.h"
#include "EditorStyle.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include "Engine/FProject.h"
#include "Engine/FAssetFile.h"
#include "Engine/UTexture2D.h"
#include "Engine/UMaterial.h"
#include "Engine/Component/ULightComponentBase.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Renderer/Renderer.h"
#include "Engine/SceneSerializer.h"
#include "Engine/FWorldContext.h"
#include "Editor/FAssetDragDrop.h"
#include "Editor/FMeshViewer.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FGameExporter.h"
#include "Editor/FProjectDialog.h"

#include <d3d11.h>
#include <commdlg.h>
#include <shellapi.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <algorithm>
#include <array>

/**
 * @brief ImGui Win32 백엔드의 외부 메시지 처리 진입점입니다.
 * @param[in] Window 메시지를 받은 Win32 창입니다.
 * @param[in] Message Win32 메시지 번호입니다.
 * @param[in] WParam Windows 메시지의 WPARAM 인수입니다.
 * @param[in] LParam Windows 메시지의 LPARAM 인수입니다.
 * @return ImGui 백엔드의 메시지 처리 결과입니다.
 */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

namespace Editor {

    /**
     * @brief 편집 컨텍스트를 패널에 연결하고 독립 뷰포트의 배치 연결을 구성합니다.
     * @param[in] PlaySession 실행 제어와 조회에 사용할 PIE 세션의 비소유 주소입니다.
     */
    Editor::FEditorUI::FEditorUI(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession)
		: mContext{Context},
		  mViewport{Viewport},
		  mPlaySession{PlaySession},
		  mViewportLayout{Context, Viewport, PlaySession},
		  mPlaceActorsPanel{Context},
		  mOutlinerPanel{Context},
		  mDetailsPanel{Context},
		  mConsolePanel{Context} {
    }

    /**
     * @brief UI 백엔드와 표시 리소스를 정리합니다.
     */
    Editor::FEditorUI::~FEditorUI() {
        Shutdown();
    }

    /**
     * @brief 독립 뷰포트를 배치할 ImGui 백엔드를 준비합니다.
     */
    bool Editor::FEditorUI::Initialize(RenderCore::IRendererModule* Renderer, Platform::FWindowsApplication* Platform) {
        if (Renderer == nullptr || Platform == nullptr || mContextCreated || mContext->GetProject() == nullptr) {
            return false;
        }

        mPlatform = Platform;
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        mContextCreated = true;

        ImGuiIO& Input{ImGui::GetIO()};
        Input.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

        const std::filesystem::path Layout{mContext->GetProject()->GetRootPath() / "Saved/EditorLayout.ini"};
        std::error_code LayoutError{};
        std::filesystem::create_directories(Layout.parent_path(), LayoutError);
        mIniPath = Core::FilePath::ToUtf8(Layout);
        Input.IniFilename = mIniPath.c_str();
        Input.LogFilename = nullptr;

        if (std::filesystem::exists("C:/Windows/Fonts/malgun.ttf")) {
            Input.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/malgun.ttf", 16.0f);
        } else if (std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf")) {
            Input.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f);
        } else {
            Input.Fonts->AddFontDefault();
        }

        ImGui::StyleColorsDark();

        ImGuiStyle& Style{ImGui::GetStyle()};
        Style.WindowRounding = 3.0f;
        Style.FrameRounding = 3.0f;
        Style.GrabRounding = 3.0f;
        mUIScale.Initialize(16.0f);
        mUIScale.Update(*Platform);

        mWin32Ready = ImGui_ImplWin32_Init(Platform->GetWindowHandle());
        mDxReady = mWin32Ready && ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(Renderer->GetNativeDevice()), static_cast<ID3D11DeviceContext*>(Renderer->GetNativeContext()));

        if (!mDxReady || !mViewportLayout.Initialize(Renderer)) {
            Shutdown();

            return false;
        }

        Platform->SetMessageHandler([this](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
            if (Message == WM_DROPFILES) {
                QueueFileDrop(reinterpret_cast<void*>(WParam));
                return true;
            }

            return ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam) != 0;
        });

        mInitialized = true;
        ImGui::LoadIniSettingsFromDisk(mIniPath.c_str());
        InitializeSettings();
        InstallEditorIcons();

        if (mContext->GetWorld() != nullptr && mContext->GetWorld()->GetPersistentLevel() != nullptr) {
            for (const auto& Actor : mContext->GetWorld()->GetPersistentLevel()->GetActors()) {
                for (const auto& Component : Actor->GetComponents()) {
                    if (auto* Light{CoreUObject::Cast<Engine::ULightComponentBase>(Component.get())}) {
                        Light->SynchronizeLight();
                    }
                }
            }
        }

        mAssetCatalog = std::make_unique<Editor::FEditorAssetCatalog>(mContext->GetProject()->GetAssets());
        mThumbnails = std::make_unique<Editor::FAssetThumbnailCache>(mContext->GetProject()->GetAssets(), Renderer);
        mDetailsPanel.SetAssetServices(mAssetCatalog.get(), mThumbnails.get());
        mMeshViewer = std::make_unique<Editor::FMeshViewer>(Renderer, mContext->GetProject()->GetAssets());
        mExit = false;
        mRefreshContent = true;
        RefreshContent();
        DragAcceptFiles(Platform->GetWindowHandle(), TRUE);

        return true;
    }

    void Editor::FEditorUI::InstallEditorIcons() {
        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
        const std::filesystem::path Bundled{Length > 0 && Length < Executable.size() ? std::filesystem::path{Executable.data()}.parent_path() / "Templates/Project/Content/Editor/Icon" : std::filesystem::path{}};
        Engine::FAssetManager* Assets{mContext->GetProject()->GetAssets()};

        for (const char* Name : {"SpotLight_64x.png", "PointLight_64x.png", "DirectionalLight_64x.png", "SkyLight_64x.png", "Pawn_64x.png", "ExponentialHeightFog_64x.png", "Sources.txt"}) {
            Core::FString Error{};
            const std::filesystem::path Target{Assets->ResolvePath(std::filesystem::path{"Editor/Icon"} / Name, Error)};
            std::error_code Code{};

            if (Target.empty()) {
                mContext->AddLog(Error);
                continue;
            }

            if (std::filesystem::is_regular_file(Target, Code)) {
                continue;
            }

            std::filesystem::path Source{Bundled / Name};

            if (Bundled.empty() || !std::filesystem::is_regular_file(Source, Code)) {
                Source = std::filesystem::path{"Templates/Project/Content/Editor/Icon"} / Name;
            }

            if (!std::filesystem::is_regular_file(Source, Code)) {
                mContext->AddLog("Cannot find bundled editor icon: " + Core::FString{Name});
                continue;
            }

            std::filesystem::create_directories(Target.parent_path(), Code);

            if (!Code) {
                std::filesystem::copy_file(Source, Target, std::filesystem::copy_options::skip_existing, Code);
            }

            if (Code) {
                mContext->AddLog("Cannot install editor icon '" + Core::FString{Name} + "': " + Code.message());
            }
        }
    }

    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Editor::FEditorUI::Shutdown() {
        if (mInitialized) {
            SaveSettings();
        }

        if (mPlatform != nullptr) {
            const HWND Window{mPlatform->GetWindowHandle()};

            if (Window != nullptr) {
                DragAcceptFiles(Window, FALSE);

                MSG Message{};

                while (PeekMessageW(&Message, Window, WM_DROPFILES, WM_DROPFILES, PM_REMOVE)) {
                    DragFinish(reinterpret_cast<HDROP>(Message.wParam));
                }
            }
        }

        mDroppedFiles.clear();
        mDropDirectories.clear();
        mContentDropDirectories.clear();
        mContentDropBounds.clear();
        mMeshViewer.reset();
        mDetailsPanel.SetAssetServices(nullptr, nullptr);
        mThumbnails.reset();
        mAssetCatalog.reset();
        mContentSearch.fill('\0');
        mContentFilter.reset();

        if (mContextCreated && ImGui::GetIO().IniFilename != nullptr) {
            if (ImGui::GetCurrentContext()->SettingsLoaded) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }

            ImGui::GetIO().IniFilename = nullptr;
        }

        mViewportLayout.Shutdown();

        if (mPlatform != nullptr) {
            mPlatform->SetMessageHandler({});
        }

        if (mDxReady) {
            ImGui_ImplDX11_Shutdown();
            mDxReady = false;
        }

        if (mWin32Ready) {
            ImGui_ImplWin32_Shutdown();

            mWin32Ready = false;
        }

        if (mContextCreated) {
            ImGui::DestroyContext();
            mContextCreated = false;
        }

        mInitialized = false;
        mAssetPaths.clear();
        mContentFolders.clear();
        mAssetTypes.clear();
        mContentDirectory.clear();
        mSelectedContentPath.clear();
        mContentError.clear();
        mRevealContentDirectory = true;
        mPreviewTexture = nullptr;
        mPreviewTexturePath.clear();
        mFocusTexturePreview = false;
        mEditingMaterial = nullptr;
        mMaterialPath.clear();
        mMaterialError.clear();
        mMaterialDirty = false;
        mPlatform = nullptr;
        mSettingsOpen = false;
        mFocusSettings = false;
        mSettingsDirty = false;

        mExit = true;
    }

    /**
     * @brief 프레임 입력과 패널 UI를 구성합니다.
     */
    bool Editor::FEditorUI::BeginFrame(float DeltaTime) {
        static Core::FStat UiTime{"unit", "Editor UI CPU", Core::EStatUnit::Milliseconds};
        const Core::FScopedStatTimer StatTimer{UiTime};

        if (!mInitialized) {
            return false;
        }

        mContext->GetRenderer()->BeginFrame(mPlatform->GetWidth(), mPlatform->GetHeight());

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        if (mAppearanceDirty) {
            mUIScale.SetBaseStyle(mThemes.GetStyle(mThemeIndex), mFontSize);
            mAppearanceDirty = false;
        }

        mUIScale.Update(*mPlatform);
        ImGui::NewFrame();
        mThumbnails->BeginFrame();
        ImportDroppedFiles();

        if (mRefreshContent) {
            RefreshContent();
        }

        if (mEditingMaterial != nullptr) {
            mThumbnails->GetMaterialPreview(mEditingMaterial);
        }

        DrawMenu();
        DrawEditorToolbar();
        BuildLayout();

        Editor::FEditorContext* Inspection{mPlaySession->IsActive() ? mPlaySession->GetInspectionContext() : mContext};
        mOutlinerPanel.SetEditorContext(Inspection);
        mDetailsPanel.SetEditorContext(Inspection);
        mPlaceActorsPanel.Draw(DeltaTime);
        mOutlinerPanel.Draw(DeltaTime);
        mDetailsPanel.Draw(DeltaTime);
        mViewportLayout.DrawUI(DeltaTime);
        mConsolePanel.Draw(DeltaTime);
        GetActiveViewport()->GetStatistics()->DrawPanel(mShowStatistics);
        DrawContent();
        DrawTexturePreview();
        DrawMaterialEditor();
        DrawSettings();

        if (mSettingsDirty) {
            mSettingsSaveDelay += DeltaTime;

            if (mSettingsSaveDelay >= 2.0f) {
                SaveSettings();
                mSettingsSaveDelay = 0.0f;
            }
        }

        if (mMeshViewer != nullptr) {
            mMeshViewer->DrawUI();
        }

        return true;
    }

    /**
     * @brief 동기화한 장면과 통계를 그린 뒤 ImGui 드로 데이터를 제출합니다.
     */
    void Editor::FEditorUI::EndFrame(float DeltaTime) {
        if (!mInitialized) {
            return;
        }

        mViewportLayout.Draw(DeltaTime);
        DrawShadowAtlas();

        static Core::FStat UiSubmitTime{"unit", "ImGui submission CPU", Core::EStatUnit::Milliseconds};
        const Core::FScopedStatTimer StatTimer{UiSubmitTime};

        ImGui::Render();
        mContext->GetRenderer()->BeginUiRender();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Editor::FEditorUI::DrawShadowAtlas() {
        if (!mContext->IsShadowAtlasOpen()) {
            return;
        }

        bool Open{true};

        ImGui::SetNextWindowSize({760.0f, 840.0f}, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Shadow Atlas", &Open)) {
            const RenderCore::FShadowAtlasView Atlas{mContext->GetRenderer()->GetShadowAtlasView()};
            const RenderCore::FRenderStats Stats{mContext->GetRenderer()->GetStats()};
            ImGui::Text("%u x %u | D24S8 | %u maps", Atlas.mSize, Atlas.mSize, Stats.mShadowMaps);
            ImGui::Text("Resources: %.1f MiB | Allocated tiles: %.1f MiB", Stats.mShadowAtlasBytes / (1024.0 * 1024.0), Stats.mShadowAllocatedBytes / (1024.0 * 1024.0));
            ImGui::Text("Directional: %u | Point: %u | Spot: %u", Stats.mDirectionalLights, Stats.mPointLights, Stats.mSpotLights);
            ImGui::Text("Reduced: %u | Unallocated lights: %u", Stats.mShadowReducedLights, Stats.mShadowDroppedLights);
            ImGui::SliderFloat("Zoom", &mShadowAtlasZoom, 1.0f, 8.0f, "%.1fx");

            if (Atlas.mTexture != nullptr) {
                const float Size{std::max(256.0f, ImGui::GetContentRegionAvail().x - 20.0f) * mShadowAtlasZoom};

                if (ImGui::BeginChild("AtlasImage", {}, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
                    const ImVec2 Origin{ImGui::GetCursorScreenPos()};

                    ImGui::Image(reinterpret_cast<ImTextureID>(Atlas.mTexture), {Size, Size});

                    ImDrawList* Draw{ImGui::GetWindowDrawList()};
                    Draw->AddRect(Origin, {Origin.x + Size, Origin.y + Size}, IM_COL32(255, 230, 0, 255));

                    for (const RenderCore::FShadowAtlasTile& Tile : Atlas.mTiles) {
                        const ImVec2 Minimum{Origin.x + Tile.mRectangle.x * Size, Origin.y + Tile.mRectangle.y * Size};
                        const ImVec2 Maximum{Minimum.x + Tile.mRectangle.z * Size, Minimum.y + Tile.mRectangle.w * Size};
                        Draw->AddRect(Minimum, Maximum, IM_COL32(255, 230, 0, 255), 0.0f, 0, 1.5f);

                        if (ImGui::IsMouseHoveringRect(Minimum, Maximum)) {
                            ImGui::SetTooltip("Map %u | %u x %u", Tile.mFace, Tile.mResolution, Tile.mResolution);
                        }
                    }
                }

                ImGui::EndChild();
            } else {
                ImGui::TextWrapped("No active shadow maps. Enable Shadows in a lit viewport and enable CastShadows on a light.");
            }
        }

        ImGui::End();
        mContext->SetShadowAtlasOpen(Open);
    }

    /**
     * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
     */
    bool Editor::FEditorUI::WantsExit() const {
        return mExit;
    }

    Editor::FEditorViewport* Editor::FEditorUI::GetActiveViewport() const {
        return mViewportLayout.GetActiveViewport();
    }

    /**
     * @brief 장면 로드 또는 저장용 Windows 파일 대화상자를 표시합니다.
     */
    std::filesystem::path Editor::FEditorUI::ChooseFile(bool Save) {
        std::array<wchar_t, 4096> Buffer{};

        if (Save) {
            const std::wstring Filename{mContext->GetScenePath().empty() ? L"Scene.masset" : mContext->GetScenePath().filename().wstring()};
            std::copy_n(Filename.begin(), std::min(Filename.size(), Buffer.size() - 1), Buffer.begin());
        }

        const std::filesystem::path Directory{mContext->GetProject()->GetAssets()->GetContentRoot() / "Maps"};

        std::error_code Error{};
        std::filesystem::create_directories(Directory, Error);

        OPENFILENAMEW Dialog{};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = mPlatform->GetWindowHandle();
        Dialog.lpstrFilter = L"Macaw Scene (*.masset)\0*.masset\0";
        Dialog.lpstrFile = Buffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
        Dialog.lpstrInitialDir = Directory.c_str();
        Dialog.lpstrDefExt = L"masset";
        Dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | (Save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

        if (Save ? GetSaveFileNameW(&Dialog) : GetOpenFileNameW(&Dialog)) {
            return std::filesystem::path{Buffer.data()};
        }

        return {};
    }

    /**
     * @brief 에디터 패널의 초기 ImGui 도킹 레이아웃을 구성합니다.
     */
    void Editor::FEditorUI::BuildLayout() {
        const ImGuiID Dockspace{ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport())};

        if (ImGui::DockBuilderGetNode(Dockspace) != nullptr && ImGui::DockBuilderGetNode(Dockspace)->ChildNodes[0] == nullptr && ImGui::DockBuilderGetNode(Dockspace)->ChildNodes[1] == nullptr) {
            ImGui::DockBuilderRemoveNode(Dockspace);
            ImGui::DockBuilderAddNode(Dockspace, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(Dockspace, ImGui::GetMainViewport()->WorkSize);

            ImGuiID Center{Dockspace};
            const ImGuiID Left{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Left, 0.19f, nullptr, &Center)};
            const ImGuiID Right{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Right, 0.32f, nullptr, &Center)};
            const ImGuiID Bottom{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Down, 0.24f, nullptr, &Center)};

            ImGuiID Outliner{};
            const ImGuiID Placement{ImGui::DockBuilderSplitNode(Left, ImGuiDir_Down, 0.55f, nullptr, &Outliner)};
            ImGui::DockBuilderDockWindow(mPlaceActorsPanel.GetName().c_str(), Placement);
            ImGui::DockBuilderDockWindow(mOutlinerPanel.GetName().c_str(), Outliner);
            ImGui::DockBuilderDockWindow(mDetailsPanel.GetName().c_str(), Right);
            ImGui::DockBuilderDockWindow(mConsolePanel.GetName().c_str(), Bottom);
            ImGui::DockBuilderDockWindow("Content", Bottom);
            ImGui::DockBuilderDockWindow(mViewport->GetName().c_str(), Center);

            ImGui::DockBuilderFinish(Dockspace);
        }
    }

    void Editor::FEditorUI::DrawEditorToolbar() {
        Editor::EditorStyle::PushViewportToolbar();

        const float Scale{mUIScale.GetScale()};
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8.0f * Scale, 4.0f * Scale});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));

        const float Height{ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f};
        const ImGuiWindowFlags Flags{ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing};

        if (ImGui::BeginViewportSideBar("##EditorToolbar", ImGui::GetMainViewport(), ImGuiDir_Up, Height, Flags)) {
            DrawPlayControls();
            ImGui::SameLine(0.0f, 12.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s%s%s", mPlaySession->IsActive() ? mPlaySession->IsPaused() ? "[Paused] " : "[Playing] " : "", mContext->GetScenePath().empty() ? "Untitled" : Core::FilePath::ToUtf8(mContext->GetScenePath().filename()).c_str(), mContext->IsSceneDirty() ? " *" : "");
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        Editor::EditorStyle::PopViewportToolbar();
    }

    void Editor::FEditorUI::DrawPlayControls() {
        ImGui::PushID("PIEControls");

        const bool Active{mPlaySession->IsActive()};
        const bool Paused{mPlaySession->IsPaused()};
        const bool CanStart{!Active && mViewportLayout.CanChangeLayout() && mPlaySession->CanStart()};
        const char* PlayTooltip{Active ? "Stop (Esc)" : CanStart ? "Play (Alt+P)" : "Play requires an active world camera and an idle editor"};

        if (Editor::EditorStyle::IconButton("PlayStop", Active ? Editor::EditorStyle::EIcon::Stop : Editor::EditorStyle::EIcon::Play, false, Active || CanStart, false, PlayTooltip)) {
            if (Active) {
                mPlaySession->Stop();
            } else {
                mPlaySession->Start();
            }
        }

        ImGui::SameLine(0.0f, 4.0f);

        if (Editor::EditorStyle::IconButton("PauseResume", Paused ? Editor::EditorStyle::EIcon::Play : Editor::EditorStyle::EIcon::Pause, Paused, Active, false, Paused ? "Resume" : "Pause")) {
            mPlaySession->SetPaused(!Paused);
        }

        ImGui::SameLine(0.0f, 4.0f);

        if (Editor::EditorStyle::IconButton("Step", Editor::EditorStyle::EIcon::Step, false, Active && Paused, false, "Advance 1 Tick")) {
            mPlaySession->Step();
        }

        ImGui::PopID();
    }

    /**
     * @brief 장면 파일과 패널 표시 메뉴를 그립니다.
     */
    void Editor::FEditorUI::DrawMenu() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::BeginDisabled(!mContext->CanEditWorld());

                if (ImGui::MenuItem("New Project...")) {
                    ChooseProject(true);
                }

                if (ImGui::MenuItem("Open Project...")) {
                    ChooseProject(false);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                    if (ConfirmSceneChange()) {
                        mContext->NewScene();
                    }
                }

                if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                    const auto Path{ChooseFile(false)};

                    if (!Path.empty() && ConfirmSceneChange()) {
                        mContext->LoadScene(Path);
                    }
                }

                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                    const auto Path{mContext->GetScenePath().empty() ? ChooseFile(true) : mContext->GetScenePath()};

                    if (!Path.empty()) {
                        mContext->SaveScene(Path);
                        mRefreshContent = true;
                    }
                }

                if (ImGui::MenuItem("Save Scene As...")) {
                    const auto Path{ChooseFile(true)};

                    if (!Path.empty()) {
                        mContext->SaveScene(Path);
                        mRefreshContent = true;
                    }
                }

                if (ImGui::MenuItem("Set Current Scene as Startup", nullptr, false, !mContext->GetScenePath().empty())) {
                    Core::FString Error{};

                    if (!mContext->SaveScene(mContext->GetScenePath()) || !mContext->GetProject()->SetStartupScene(mContext->GetScenePath(), Error)) {
                        mContext->AddLog(Error);
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Export Game...", nullptr, false, mContext->GetProject() != nullptr)) {
                    ExportGame();
                }

                ImGui::EndDisabled();
                ImGui::Separator();

                if (ImGui::MenuItem("Exit") && ConfirmSceneChange()) {
                    mExit = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Statistics", nullptr, &mShowStatistics);

                if (ImGui::BeginMenu("Stat Overlay")) {
                    Editor::FViewportStatistics* Statistics{GetActiveViewport()->GetStatistics()};

                    for (const Core::FString& Group : Core::FStats::Get().GetGroups()) {
                        if (ImGui::MenuItem(Group.c_str(), nullptr, Statistics->IsEnabled(Group))) {
                            Core::FString Output{};
                            Statistics->ExecuteCommand("stat " + Group, Output);
                        }
                    }

                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Place Actors", nullptr, mPlaceActorsPanel.IsVisible())) {
                    mPlaceActorsPanel.SetVisible(!mPlaceActorsPanel.IsVisible());
                }

                if (ImGui::MenuItem("Four Viewports", nullptr, mViewportLayout.IsQuadView(), mViewportLayout.CanChangeLayout())) {
                    mViewportLayout.SetQuadView(!mViewportLayout.IsQuadView());
                }

                if (ImGui::MenuItem(mConsolePanel.GetName().c_str(), nullptr, mConsolePanel.IsVisible())) {
                    mConsolePanel.SetVisible(!mConsolePanel.IsVisible());
                }

                ImGui::EndMenu();
            }

            DrawSettingsMenu();
            ImGui::EndMainMenuBar();
        }

        const ImGuiIO& Input{ImGui::GetIO()};

        if (Input.KeyCtrl && !Input.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Comma, false)) {
            mSettingsOpen = true;
            mFocusSettings = true;
        }

        if (!Input.WantTextInput && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) {
            if (Input.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_P, false) && mViewportLayout.CanChangeLayout()) {
                mPlaySession->Start();
            } else if (mPlaySession->IsActive() && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
                mPlaySession->Stop();
            }
        }

        if (Input.KeyCtrl && !Input.WantTextInput && mContext->CanEditWorld()) {
            if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                const auto Path{mContext->GetScenePath().empty() ? ChooseFile(true) : mContext->GetScenePath()};

                if (!Path.empty()) {
                    mContext->SaveScene(Path);
                    mRefreshContent = true;
                }
            } else if (ImGui::IsKeyPressed(ImGuiKey_O)) {
                const auto Path{ChooseFile(false)};

                if (!Path.empty() && ConfirmSceneChange()) {
                    mContext->LoadScene(Path);
                }
            } else if (ImGui::IsKeyPressed(ImGuiKey_N)) {
                if (ConfirmSceneChange()) {
                    mContext->NewScene();
                }
            }
        }
    }

    /**
     * @brief 내보낼 폴더를 고르고 저장하지 않은 장면을 확인한 뒤 현재 프로젝트를 독립 실행 게임으로 내보냅니다.
     */
    void Editor::FEditorUI::ExportGame() {
        const auto Destination{Editor::FProjectDialog::ChooseDirectory(mPlatform->GetWindowHandle())};

        if (Destination.empty()) {
            return;
        }

        if (!ConfirmSceneChange()) {
            return;
        }

        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
        Core::FString Error{};
        bool Exported{};

        if (Length == 0 || Length >= Executable.size()) {
            Error = "Cannot locate the game executable";
        } else {
            const auto Runtime{std::filesystem::path{Executable.data()}.parent_path() / "GameRuntime"};
            Exported = Editor::FGameExporter::Export(*mContext->GetProject(), Runtime, Destination, Error);
        }

        const Core::FString Message{Exported ? "Exported game to " + Core::FilePath::ToUtf8(Destination) : Error};
        mContext->AddLog(Message);
        MessageBoxW(mPlatform->GetWindowHandle(), Core::FilePath::FromUtf8(Message).c_str(), Exported ? L"Game Exported" : L"Cannot Export Game", MB_OK | (Exported ? MB_ICONINFORMATION : MB_ICONERROR));
    }

    bool Editor::FEditorUI::ConfirmSceneChange() {
        if (!ConfirmMaterialChange()) {
            return false;
        }

        if (!mContext->IsSceneDirty()) {
            return true;
        }

        const Core::Int32 Choice{MessageBoxW(mPlatform->GetWindowHandle(), L"Save changes to the current scene?", L"Scene", MB_YESNOCANCEL | MB_ICONQUESTION)};

        if (Choice == IDCANCEL) {
            return false;
        }

        if (Choice == IDNO) {
            return true;
        }

        const auto Path{mContext->GetScenePath().empty() ? ChooseFile(true) : mContext->GetScenePath()};
        const bool Saved{!Path.empty() && mContext->SaveScene(Path)};
        mRefreshContent = mRefreshContent || Saved;

        return Saved;
    }

    void Editor::FEditorUI::ChooseProject(bool Create) {
        if (!ConfirmSceneChange()) {
            return;
        }

        mContext->RequestProject({}, Create);
    }

    void Editor::FEditorUI::ImportAsset() {
        std::array<wchar_t, 32768> Buffer{};
        OPENFILENAMEW Dialog{};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = mPlatform->GetWindowHandle();
        Dialog.lpstrTitle = L"Import Asset";
        Dialog.lpstrFilter = L"Supported Assets\0*.obj;*.fbx;*.gltf;*.glb;*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds;*.hdr\0";
        Dialog.lpstrFile = Buffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
        Dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&Dialog)) {
            ImportFiles({std::filesystem::path{Buffer.data()}}, mContentDirectory);
        }
    }

    void Editor::FEditorUI::ImportFiles(const Core::TArray<std::filesystem::path>& Sources, const std::filesystem::path& Directory) {
        if (!mContext->CanEditWorld()) {
            return;
        }

        const std::filesystem::path Destination{Directory};
        Engine::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
        std::filesystem::path LastImported{};
        Core::FString Failures{};
        Core::Uint32 Imported{};

        for (const auto& Source : Sources) {
            std::filesystem::path Target{};
            Core::FString Error{};

            if (!Assets->ImportAsset(Source, Destination, Target, Error)) {
                const Core::FString Failure{"Cannot import '" + Core::FilePath::ToUtf8(Source.filename()) + "': " + Error};
                mContext->AddLog(Failure);
                Failures += (Failures.empty() ? Core::FString{} : "\n") + Failure;
                continue;
            }

            ++Imported;
            LastImported = Target;
            mContext->AddLog("Imported " + Core::FilePath::ToGenericUtf8(Target));
        }

        if (Imported != 0) {
            mContentFilter.reset();
            mContentSearch.fill('\0');
            RefreshContent();
            NavigateContent(Imported == 1 ? LastImported.parent_path() : Destination);
            mSelectedContentPath = Imported == 1 ? LastImported : LastImported.parent_path();
        }

        mContentError = std::move(Failures);
    }

    void Editor::FEditorUI::QueueFileDrop(void* Handle) {
        const HDROP Drop{static_cast<HDROP>(Handle)};
        POINT Point{};

        if (Drop == nullptr) {
            return;
        }

        ImGuiContext* Context{ImGui::GetCurrentContext()};

        if (!mInitialized || !mContext->CanEditWorld() || Context == nullptr || !DragQueryPoint(Drop, &Point) || Context->OpenPopupStack.Size != 0) {
            DragFinish(Drop);
            return;
        }

        if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
            ClientToScreen(mPlatform->GetWindowHandle(), &Point);
        }

        const ImVec2 Position{static_cast<float>(Point.x), static_cast<float>(Point.y)};
        ImGuiWindow* Browser{ImGui::FindWindowByName("Content")};
        ImGuiWindow* Hovered{};

        for (int Index{Context->Windows.Size}; Index > 0; --Index) {
            ImGuiWindow* Window{Context->Windows[Index - 1]};

            if (Window->WasActive && !Window->Hidden && (Window->Flags & ImGuiWindowFlags_NoMouseInputs) == 0 && Window->OuterRectClipped.Contains(Position)) {
                Hovered = Window;
                break;
            }
        }

        if (Browser == nullptr || Browser->Collapsed || Hovered == nullptr || !ImGui::IsWindowChildOf(Hovered, Browser, false, false)) {
            DragFinish(Drop);
            return;
        }

        std::filesystem::path Directory{};
        bool Found{};

        for (std::size_t Index{mContentDropBounds.size()}; Index > 0; --Index) {
            const Core::FVector4& Bounds{mContentDropBounds[Index - 1]};

            if (Position.x >= Bounds.x && Position.y >= Bounds.y && Position.x < Bounds.z && Position.y < Bounds.w) {
                Directory = mContentDropDirectories[Index - 1];
                Found = true;
                break;
            }
        }

        if (!Found) {
            DragFinish(Drop);
            return;
        }

        Core::TArray<std::filesystem::path> Sources{};
        const UINT Count{DragQueryFileW(Drop, 0xFFFFFFFF, nullptr, 0)};

        for (UINT Index{}; Index < Count; ++Index) {
            const UINT Length{DragQueryFileW(Drop, Index, nullptr, 0)};
            std::wstring Path{};
            Path.resize(static_cast<std::size_t>(Length) + 1);

            if (Length != 0 && DragQueryFileW(Drop, Index, Path.data(), Length + 1) == Length) {
                Path.resize(Length);
                Sources.emplace_back(Path);
            }
        }

        DragFinish(Drop);

        if (!Sources.empty()) {
            mDropDirectories.push_back(std::move(Directory));
            mDroppedFiles.push_back(std::move(Sources));
        }
    }

    void Editor::FEditorUI::ImportDroppedFiles() {
        auto Files{std::move(mDroppedFiles)};
        auto Directories{std::move(mDropDirectories)};
        mDroppedFiles.clear();
        mDropDirectories.clear();

        for (std::size_t Index{}; Index < Files.size(); ++Index) {
            ImportFiles(Files[Index], Directories[Index]);
        }
    }

    void Editor::FEditorUI::RegisterContentDropTarget(const std::filesystem::path& Directory, const Core::FVector4& Bounds) {
        const ImRect Clip{ImGui::GetCurrentWindow()->ClipRect};
        const Core::FVector4 Visible{std::max(Bounds.x, Clip.Min.x), std::max(Bounds.y, Clip.Min.y), std::min(Bounds.z, Clip.Max.x), std::min(Bounds.w, Clip.Max.y)};

        if (Visible.z > Visible.x && Visible.w > Visible.y) {
            mContentDropDirectories.push_back(Directory);
            mContentDropBounds.push_back(Visible);
        }
    }

    void Editor::FEditorUI::DrawContent() {
        mContentDropDirectories.clear();
        mContentDropBounds.clear();

        if (ImGui::Begin("Content")) {
            const ImVec2 Minimum{ImGui::GetWindowPos()};
            const ImVec2 Size{ImGui::GetWindowSize()};
            RegisterContentDropTarget(mContentDirectory, {Minimum.x, Minimum.y, Minimum.x + Size.x, Minimum.y + Size.y});
            ImGui::TextUnformatted(mContext->GetProject()->GetName().c_str());
            ImGui::BeginDisabled(!mContext->CanEditWorld());

            if (ImGui::Button("Import...")) {
                ImportAsset();
            }

            ImGui::SameLine();

            if (ImGui::Button("New Material")) {
                CreateMaterial();
            }

            ImGui::EndDisabled();
            ImGui::SameLine();

            if (ImGui::Button("Refresh")) {
                mRefreshContent = true;
            }

            ImGui::Separator();
            DrawContentBreadcrumbs();
            ImGui::SetNextItemWidth(120.0f);

            const char* FilterName{!mContentFilter.has_value() ? "All Assets" : *mContentFilter == Engine::EAssetType::Mesh ? "Meshes" : *mContentFilter == Engine::EAssetType::Material ? "Materials" : *mContentFilter == Engine::EAssetType::Texture ? "Textures" : "Scenes"};

            if (ImGui::BeginCombo("##AssetType", FilterName)) {
                if (ImGui::Selectable("All Assets", !mContentFilter.has_value())) {
                    mContentFilter.reset();
                }

                for (Engine::EAssetType Type : {Engine::EAssetType::Mesh, Engine::EAssetType::Material, Engine::EAssetType::Texture, Engine::EAssetType::Scene}) {
                    const char* Name{Type == Engine::EAssetType::Mesh ? "Meshes" : Type == Engine::EAssetType::Material ? "Materials" : Type == Engine::EAssetType::Texture ? "Textures" : "Scenes"};

                    if (ImGui::Selectable(Name, mContentFilter == Type)) {
                        mContentFilter = Type;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x));
            ImGui::InputTextWithHint("##ContentSearch", "Search assets...", mContentSearch.data(), mContentSearch.size());

            if (!mContentError.empty()) {
                ImGui::TextWrapped("%s", mContentError.c_str());
            }

            const float Height{std::max(64.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing())};

            if (ImGui::BeginTable("Content Browser", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 190.0f);
                ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::BeginChild("Content Folders", ImVec2{0.0f, Height});
                DrawContentTree({});
                mRevealContentDirectory = false;
                ImGui::EndChild();
                ImGui::TableSetColumnIndex(1);
                ImGui::BeginChild("Content Files", ImVec2{0.0f, Height});
                DrawContentEntries();
                ImGui::EndChild();
                ImGui::EndTable();
            }

            ImGui::TextDisabled("Drop files here to import. Double-click to open. Drag thumbnails onto Details to assign.");
        }

        ImGui::End();
    }

    void Editor::FEditorUI::DrawTexturePreview() {
        if (mPreviewTexture == nullptr || !mPreviewTexture->HasResources()) {
            return;
        }

        bool Open{true};
        const float UIScale{mUIScale.GetScale()};
        ImGui::SetNextWindowSize({640.0f * UIScale, 560.0f * UIScale}, ImGuiCond_FirstUseEver);

        if (mFocusTexturePreview) {
            ImGui::SetNextWindowFocus();
            ImGui::SetNextWindowCollapsed(false);
            mFocusTexturePreview = false;
        }

        if (ImGui::Begin("Texture Preview", &Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGui::TextUnformatted(Core::FilePath::ToGenericUtf8(mPreviewTexturePath).c_str());
            ImGui::Text("Size: %u x %u", mPreviewTexture->GetSizeX(), mPreviewTexture->GetSizeY());
            ImGui::Separator();

            const ImVec2 Available{ImGui::GetContentRegionAvail()};

            if (Available.x > 0.0f && Available.y > 0.0f) {
                const float SizeX{static_cast<float>(mPreviewTexture->GetSizeX())};
                const float SizeY{static_cast<float>(mPreviewTexture->GetSizeY())};
                const float Scale{std::min(Available.x / SizeX, Available.y / SizeY)};
                const ImVec2 ImageSize{SizeX * Scale, SizeY * Scale};
                const ImVec2 Origin{ImGui::GetCursorPos()};
                ImGui::SetCursorPos({Origin.x + (Available.x - ImageSize.x) * 0.5f, Origin.y + (Available.y - ImageSize.y) * 0.5f});
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mPreviewTexture->GetShaderResourceView()))}, ImageSize);
            }
        }

        ImGui::End();

        if (!Open) {
            mPreviewTexture = nullptr;
            mPreviewTexturePath.clear();
        }
    }

    void Editor::FEditorUI::RefreshContent() {
        mAssetPaths.clear();
        mAssetTypes.clear();
        mContentFolders.clear();
        mContentFolders[{}] = {};
        mContentError.clear();

        if (mAssetCatalog == nullptr) {
            return;
        }

        mAssetCatalog->Refresh(mContentError);

        for (const auto& Directory : mAssetCatalog->GetDirectories()) {
            mContentFolders[Directory.parent_path()].push_back(Directory);
            mContentFolders.try_emplace(Directory);
        }

        for (const auto& Entry : mAssetCatalog->GetEntries()) {
            mAssetPaths.push_back(Entry.GetPath());

            if (Entry.GetType().has_value()) {
                mAssetTypes.emplace(Entry.GetPath(), *Entry.GetType());
            }
        }

        for (auto& Entry : mContentFolders) {
            std::sort(Entry.second.begin(), Entry.second.end());
            Entry.second.erase(std::unique(Entry.second.begin(), Entry.second.end()), Entry.second.end());
        }

        while (!mContentDirectory.empty() && !mContentFolders.contains(mContentDirectory)) {
            mContentDirectory = mContentDirectory.parent_path();
        }

        mThumbnails->Invalidate();
        mRefreshContent = false;
        mRevealContentDirectory = true;
    }

    void Editor::FEditorUI::NavigateContent(const std::filesystem::path& Directory) {
        if (mContentFolders.contains(Directory)) {
            mContentDirectory = Directory;
            mSelectedContentPath.clear();
            mRevealContentDirectory = true;
        }
    }

    void Editor::FEditorUI::DrawContentTree(const std::filesystem::path& Directory) {
        const auto Entry{mContentFolders.find(Directory)};

        if (Entry == mContentFolders.end()) {
            return;
        }

        const Core::FString Label{Directory.empty() ? "Content" : Core::FilePath::ToUtf8(Directory.filename())};
        const Core::FString Id{Core::FilePath::ToUtf8(Directory)};
        ImGuiTreeNodeFlags Flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth};

        if (Directory == mContentDirectory) {
            Flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (Entry->second.empty()) {
            Flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (Directory.empty()) {
            Flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        const auto Relative{mContentDirectory.lexically_relative(Directory)};

        if (mRevealContentDirectory && (Directory.empty() || Directory == mContentDirectory || (!Relative.empty() && *Relative.begin() != L".."))) {
            ImGui::SetNextItemOpen(true);
        }

        ImGui::PushID(Id.c_str());

        const bool Open{ImGui::TreeNodeEx(Label.c_str(), Flags)};
        const ImVec2 Minimum{ImGui::GetItemRectMin()};
        const ImVec2 Maximum{ImGui::GetItemRectMax()};
        RegisterContentDropTarget(Directory, {Minimum.x, Minimum.y, Maximum.x, Maximum.y});

        if (ImGui::IsItemClicked()) {
            NavigateContent(Directory);
        }

        if (Open) {
            for (const auto& Child : Entry->second) {
                DrawContentTree(Child);
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void Editor::FEditorUI::DrawContentBreadcrumbs() {
        ImGui::PushID("Content Path");
        ImGui::BeginDisabled(mContentDirectory.empty());

        if (ImGui::SmallButton("Up")) {
            NavigateContent(mContentDirectory.parent_path());
        }

        ImGui::EndDisabled();
        ImGui::SameLine();

        if (ImGui::SmallButton("Content")) {
            NavigateContent({});
        }

        const std::filesystem::path Current{mContentDirectory};
        std::filesystem::path Path{};

        for (const auto& Part : Current) {
            Path /= Part;

            const Core::FString Label{Core::FilePath::ToUtf8(Part)};
            const float Right{ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x};

            if (Right - ImGui::GetItemRectMax().x > ImGui::CalcTextSize(Label.c_str()).x + 48.0f) {
                ImGui::SameLine();
            }

            ImGui::TextUnformatted("/");
            ImGui::SameLine();

            const Core::FString Id{Core::FilePath::ToUtf8(Path)};
            ImGui::PushID(Id.c_str());

            if (ImGui::SmallButton(Label.c_str())) {
                NavigateContent(Path);
            }

            ImGui::PopID();
        }

        ImGui::PopID();
    }

    void Editor::FEditorUI::DrawContentEntries() {
        const ImGuiTextFilter Search{mContentSearch.data()};
        const bool Recursive{mContentFilter.has_value() || mContentSearch[0] != '\0'};
        Core::TArray<std::filesystem::path> Folders{};
        Core::TArray<std::filesystem::path> Assets{};
        const auto Folder{mContentFolders.find(mContentDirectory)};

        if (!Recursive && Folder != mContentFolders.end()) {
            Folders = Folder->second;
        }

        for (const auto& Path : mAssetPaths) {
            const auto Type{mAssetTypes.find(Path)};
            const auto Relative{Path.lexically_relative(mContentDirectory)};
            const bool InFolder{Recursive ? mContentDirectory.empty() || (!Relative.empty() && *Relative.begin() != L"..") : Path.parent_path() == mContentDirectory};

            if (!InFolder || (mContentFilter.has_value() && (Type == mAssetTypes.end() || Type->second != *mContentFilter)) || !Search.PassFilter(Core::FilePath::ToGenericUtf8(Path).c_str())) {
                continue;
            }

            Assets.push_back(Path);
        }

        const int Count{static_cast<int>(Folders.size() + Assets.size())};

        if (Count == 0) {
            ImGui::TextDisabled("No assets match this folder or search.");
            return;
        }

        const float Scale{mUIScale.GetScale()};
        const float Padding{4.0f * Scale};
        const bool Compact{ImGui::GetContentRegionAvail().y < 164.0f * Scale};
        const float TileWidth{(Compact ? 180.0f : 120.0f) * Scale};
        const float ImageSize{(Compact ? 48.0f : 96.0f) * Scale};
        const float ImageOffset{(Compact ? 4.0f : 10.0f) * Scale};
        const float TextX{Compact ? ImageOffset + ImageSize + 8.0f * Scale : Padding};
        const float TypeY{Compact ? Padding : ImageSize + Padding * 2.0f};
        const float NameY{TypeY + ImGui::GetTextLineHeightWithSpacing()};
        const float NameHeight{ImGui::GetTextLineHeight() * 2.0f + ImGui::GetStyle().ItemSpacing.y};
        const float TileHeight{std::max(Padding + ImageSize, NameY + NameHeight) + Padding * 2.0f};
        const int Columns{std::clamp(static_cast<int>(ImGui::GetContentRegionAvail().x / (TileWidth + 8.0f * Scale)), 1, 12)};
        bool Navigate{};
        std::filesystem::path Requested{};

        if (ImGui::BeginTable("ContentGrid", Columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX)) {
            for (int Column{}; Column < Columns; ++Column) {
                ImGui::TableSetupColumn("##AssetColumn", ImGuiTableColumnFlags_WidthFixed, TileWidth);
            }

            ImGuiListClipper Clipper{};
            Clipper.Begin((Count + Columns - 1) / Columns, TileHeight);

            while (Clipper.Step()) {
                for (int Row{Clipper.DisplayStart}; Row < Clipper.DisplayEnd; ++Row) {
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, TileHeight);

                    for (int Column{}; Column < Columns; ++Column) {
                        const int Index{Row * Columns + Column};

                        if (Index >= Count) {
                            break;
                        }

                        ImGui::TableSetColumnIndex(Column);

                        const bool Directory{static_cast<std::size_t>(Index) < Folders.size()};
                        const auto& Path{Directory ? Folders[Index] : Assets[Index - Folders.size()]};
                        const auto Type{mAssetTypes.find(Path)};
                        const bool HasThumbnail{!Directory && Type != mAssetTypes.end() && (Type->second == Engine::EAssetType::Mesh || Type->second == Engine::EAssetType::Material || Type->second == Engine::EAssetType::Texture)};
                        void* Thumbnail{HasThumbnail ? mThumbnails->GetThumbnail(Path, Type->second) : nullptr};
                        const char* Kind{Directory ? "Folder" : Type == mAssetTypes.end() ? "File" : Type->second == Engine::EAssetType::Mesh ? "Mesh" : Type->second == Engine::EAssetType::Material ? "Material" : Type->second == Engine::EAssetType::Texture ? "Texture" : "Scene"};
                        const Core::FString Actual{Core::FilePath::ToGenericUtf8(Path)};
                        const Core::FString Name{Core::FilePath::ToUtf8(Directory ? Path.filename() : Path.stem())};
                        ImGui::PushID(Actual.c_str());

                        const ImVec2 Origin{ImGui::GetCursorScreenPos()};

                        if (ImGui::Selectable("##AssetTile", mSelectedContentPath == Path, ImGuiSelectableFlags_AllowDoubleClick, {TileWidth - Padding, TileHeight - Padding * 2.0f})) {
                            mSelectedContentPath = Path;

                            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                if (Directory) {
                                    Requested = Path;
                                    Navigate = true;
                                } else if (Type != mAssetTypes.end() && mContext->CanEditWorld()) {
                                    OpenContentAsset(Path, Type->second);
                                }
                            }
                        }

                        const bool Hovered{ImGui::IsItemHovered()};

                        if (Directory) {
                            const ImVec2 Minimum{ImGui::GetItemRectMin()};
                            const ImVec2 Maximum{ImGui::GetItemRectMax()};
                            RegisterContentDropTarget(Path, {Minimum.x, Minimum.y, Maximum.x, Maximum.y});
                        }

                        if (HasThumbnail && mContext->CanEditWorld() && ImGui::BeginDragDropSource()) {
                            const char* Payload{Type->second == Engine::EAssetType::Mesh ? Editor::EditorAssetDragDrop::MeshPayload : Type->second == Engine::EAssetType::Material ? Editor::EditorAssetDragDrop::MaterialPayload : Editor::EditorAssetDragDrop::TexturePayload};
                            ImGui::SetDragDropPayload(Payload, Actual.c_str(), Actual.size() + 1);

                            if (Thumbnail != nullptr) {
                                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Thumbnail))}, {64.0f * Scale, 64.0f * Scale});
                            }

                            ImGui::TextUnformatted(Name.c_str());
                            ImGui::EndDragDropSource();
                        }

                        if (Hovered) {
                            ImGui::BeginTooltip();
                            ImGui::TextUnformatted(Core::FilePath::ToGenericUtf8(Path).c_str());
                            ImGui::TextDisabled("%s", Kind);
                            ImGui::EndTooltip();
                        }

                        const ImVec2 Next{ImGui::GetCursorScreenPos()};

                        ImGui::SetCursorScreenPos({Origin.x + ImageOffset, Origin.y + Padding});

                        if (Thumbnail != nullptr) {
                            ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Thumbnail))}, {ImageSize, ImageSize});
                        } else {
                            ImGui::GetWindowDrawList()->AddRectFilled({Origin.x + ImageOffset, Origin.y + Padding}, {Origin.x + ImageOffset + ImageSize, Origin.y + Padding + ImageSize}, Directory ? IM_COL32(79, 65, 34, 255) : IM_COL32(34, 42, 54, 255), 5.0f * Scale);

                            if (!Compact) {
                                const ImVec2 TextSize{ImGui::CalcTextSize(Kind)};
                                ImGui::GetWindowDrawList()->AddText({Origin.x + ImageOffset + (ImageSize - TextSize.x) * 0.5f, Origin.y + Padding + (ImageSize - TextSize.y) * 0.5f}, IM_COL32(200, 213, 230, 255), Kind);
                            }

                            ImGui::Dummy({ImageSize, ImageSize});
                        }

                        ImGui::SetCursorScreenPos({Origin.x + TextX, Origin.y + TypeY});
                        ImGui::TextDisabled("%s", Kind);
                        ImGui::SetCursorScreenPos({Origin.x + TextX, Origin.y + NameY});
                        ImGui::PushClipRect({Origin.x + TextX, Origin.y + NameY}, {Origin.x + TileWidth - Padding, Origin.y + NameY + NameHeight}, true);
                        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + TileWidth - TextX - 10.0f * Scale);
                        ImGui::TextWrapped("%s", Name.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::PopClipRect();
                        ImGui::SetCursorScreenPos(Next);
                        ImGui::Dummy({0.0f, 0.0f});
                        ImGui::PopID();
                    }
                }
            }

            ImGui::EndTable();
        }

        if (Navigate) {
            NavigateContent(Requested);
        }
    }

    void Editor::FEditorUI::OpenContentAsset(const std::filesystem::path& Path, Engine::EAssetType Type) {
        if (Type == Engine::EAssetType::Scene) {
            if (ConfirmSceneChange()) {
                mContext->LoadScene(Path);
            }
        } else if (Type == Engine::EAssetType::Mesh) {
            Core::FString Error{};

            if (mMeshViewer != nullptr && !mMeshViewer->Open(Path, Error)) {
                mContext->AddLog(Error);
            }
        } else if (Type == Engine::EAssetType::Material) {
            if (!ConfirmMaterialChange()) {
                return;
            }

            Engine::UMaterial* Material{mContext->GetProject()->GetAssets()->LoadMaterial(Path, mMaterialError)};

            if (Material != nullptr) {
                mEditingMaterial = Material;
                mMaterialPath = Path;
                mMaterialDirty = false;
            } else {
                mContext->AddLog(mMaterialError);
            }
        } else if (Type == Engine::EAssetType::Texture) {
            Core::FString Error{};
            Engine::UTexture2D* Texture{mContext->GetProject()->GetAssets()->LoadTexture(Path, Error)};

            if (Texture != nullptr && Texture->CreateResources(static_cast<ID3D11Device*>(mContext->GetRenderer()->GetNativeDevice()))) {
                mPreviewTexture = Texture;
                mPreviewTexturePath = Path;
                mFocusTexturePreview = true;
                mContext->AddLog("Loaded " + Core::FilePath::ToUtf8(Path));
            } else {
                mContext->AddLog(Error.empty() ? "Cannot preview the texture" : Error);
            }
        }
    }

    void Editor::FEditorUI::CreateMaterial() {
        if (!ConfirmMaterialChange()) {
            return;
        }

        Engine::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
        std::filesystem::path Path{mContentDirectory / "Material.masset"};
        std::error_code Code{};

        for (Core::Uint32 Index{1}; std::filesystem::exists(Assets->GetContentRoot() / Path, Code) && !Code; ++Index) {
            Path = mContentDirectory / ("Material_" + std::to_string(Index) + ".masset");
        }

        auto Material{CoreUObject::NewObject<Engine::UMaterial>()};
        Material->SetName(Core::FilePath::ToUtf8(Path.stem()));

        if (Code || !Assets->SaveMaterial(Path, *Material, mMaterialError)) {
            mContext->AddLog(mMaterialError.empty() ? "Cannot create the material" : mMaterialError);
            return;
        }

        mRefreshContent = true;
        mSelectedContentPath = Path;
        OpenContentAsset(Path, Engine::EAssetType::Material);
    }

    bool Editor::FEditorUI::SaveMaterial() {
        if (mEditingMaterial == nullptr || !mContext->GetProject()->GetAssets()->SaveMaterial(mMaterialPath, *mEditingMaterial, mMaterialError)) {
            return false;
        }

        mMaterialDirty = false;
        mRefreshContent = true;
        mContext->AddLog("Saved " + Core::FilePath::ToUtf8(mMaterialPath));

        return true;
    }

    bool Editor::FEditorUI::ConfirmMaterialChange() {
        if (!mMaterialDirty || mEditingMaterial == nullptr) {
            return true;
        }

        const Core::Int32 Choice{MessageBoxW(mPlatform->GetWindowHandle(), L"Save changes to the current material?", L"Material", MB_YESNOCANCEL | MB_ICONQUESTION)};

        if (Choice == IDCANCEL) {
            return false;
        }

        if (Choice == IDYES) {
            return SaveMaterial();
        }

        if (!mContext->GetProject()->GetAssets()->ReloadMaterial(mMaterialPath, mMaterialError)) {
            return false;
        }

        mMaterialDirty = false;

        return true;
    }

    bool Editor::FEditorUI::SetMaterialTexture(const Core::FString& Path) {
        Engine::FAssetManager* Assets{mContext->GetProject()->GetAssets()};

        if (!Path.empty() && Assets->LoadTexture(Core::FilePath::FromUtf8(Path), mMaterialError) == nullptr) {
            return false;
        }

        if (!mEditingMaterial->SetBaseColorTexturePath(Path) || !mEditingMaterial->ResolveTexture(*Assets, mMaterialError)) {
            if (mMaterialError.empty()) {
                mMaterialError = "Invalid texture path";
            }

            return false;
        }

        mMaterialError.clear();
        mMaterialDirty = true;

        return true;
    }

    bool Editor::FEditorUI::SetMaterialShader(const Core::FString& Path) {
        auto Candidate{CoreUObject::NewObject<Engine::UMaterial>()};
        Candidate->SetTwoSided(mEditingMaterial->IsTwoSided());

        Renderer::FRenderer* Renderer{dynamic_cast<Renderer::FRenderer*>(mContext->GetRenderer())};

        if (!Candidate->SetShaderPath(Path) || Renderer == nullptr || Renderer->GetMaterialPipeline(*Candidate) == nullptr) {
            mMaterialError = "Cannot use this material shader. Check the shader diagnostics in the log.";
            return false;
        }

        mEditingMaterial->SetShaderPath(Path);
        mMaterialError.clear();
        mMaterialDirty = true;

        return true;
    }

    void Editor::FEditorUI::DrawMaterialEditor() {
        if (mEditingMaterial == nullptr) {
            return;
        }

        bool Open{true};

        ImGui::SetNextWindowSize({560.0f, 700.0f}, ImGuiCond_FirstUseEver);

        if (ImGui::Begin(mMaterialDirty ? "Material Editor*###MaterialEditor" : "Material Editor###MaterialEditor", &Open)) {
            ImGui::TextUnformatted(Core::FilePath::ToGenericUtf8(mMaterialPath).c_str());

            if (void* Preview{mThumbnails->GetMaterialPreview(mEditingMaterial)}) {
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Preview))}, {192.0f, 192.0f});
            }

            ImGui::BeginDisabled(!mContext->CanEditWorld());

            if (ImGui::Button("Save")) {
                SaveMaterial();
            }

            ImGui::SameLine();

            if (ImGui::Button("Reload") && ConfirmMaterialChange()) {
                mContext->GetProject()->GetAssets()->ReloadMaterial(mMaterialPath, mMaterialError);
            }

            ImGui::Separator();

            if (ImGui::BeginCombo("Shader", mEditingMaterial->GetShaderPath().c_str())) {
                Core::TArray<Core::FString> Shaders{"Shaders/Lit.hlsl", "Shaders/Unlit.hlsl"};

                for (const auto& Path : mAssetPaths) {
                    const Core::FString Name{Core::FilePath::ToUtf8(Path)};

                    if (Path.extension() == L".hlsl" && Path.filename() != L"Line.hlsl" && Path.filename() != L"Gizmo.hlsl" && Path.filename() != L"Text.hlsl" && std::find(Shaders.begin(), Shaders.end(), Name) == Shaders.end()) {
                        Shaders.push_back(Name);
                    }
                }

                for (const Core::FString& Shader : Shaders) {
                    if (ImGui::Selectable(Shader.c_str(), mEditingMaterial->GetShaderPath() == Shader)) {
                        SetMaterialShader(Shader);
                    }
                }

                ImGui::EndCombo();
            }

            std::array<char, 512> ShaderPath{};
            const Core::FString& CurrentShader{mEditingMaterial->GetShaderPath()};
            std::copy_n(CurrentShader.begin(), std::min(CurrentShader.size(), ShaderPath.size() - 1), ShaderPath.begin());

            if (ImGui::InputText("Shader Path", ShaderPath.data(), ShaderPath.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                SetMaterialShader(ShaderPath.data());
            }

            auto Color{mEditingMaterial->GetBaseColor().ToArray()};

            if (ImGui::ColorEdit4("Base Color", Color.data()) && mEditingMaterial->SetBaseColor({Color[0], Color[1], Color[2], Color[3]})) {
                mMaterialDirty = true;
            }

            bool TwoSided{mEditingMaterial->IsTwoSided()};

            if (ImGui::Checkbox("Two Sided", &TwoSided)) {
                mEditingMaterial->SetTwoSided(TwoSided);
                mMaterialDirty = true;
            }

            Core::FString TexturePath{mEditingMaterial->GetBaseColorTexturePath()};

            if (mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, "Base Color Texture", TexturePath, Engine::EAssetType::Texture)) {
                SetMaterialTexture(TexturePath);
            }

            Core::FString NormalPath{mEditingMaterial->GetNormalMapTexturePath()};

            if (mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, "Normal Map", NormalPath, Engine::EAssetType::Texture)) {
                const Core::FString Previous{mEditingMaterial->GetNormalMapTexturePath()};

                if (mEditingMaterial->SetNormalMapTexturePath(NormalPath) && mEditingMaterial->ResolveTexture(*mContext->GetProject()->GetAssets(), mMaterialError)) {
                    mMaterialDirty = true;
                } else {
                    mEditingMaterial->SetNormalMapTexturePath(Previous);

                    Core::FString RestoreError{};
                    mEditingMaterial->ResolveTexture(*mContext->GetProject()->GetAssets(), RestoreError);
                }
            }

            float Specular{mEditingMaterial->GetSpecularStrength()};
            float Shininess{mEditingMaterial->GetShininess()};
            float NormalStrength{mEditingMaterial->GetNormalStrength()};

            if (ImGui::SliderFloat("Specular Strength", &Specular, 0.0f, 1.0f) && mEditingMaterial->SetSpecularStrength(Specular)) {
                mMaterialDirty = true;
            }

            if (ImGui::SliderFloat("Shininess", &Shininess, 1.0f, 1024.0f, "%.1f", ImGuiSliderFlags_Logarithmic) && mEditingMaterial->SetShininess(Shininess)) {
                mMaterialDirty = true;
            }

            if (ImGui::SliderFloat("Normal Strength", &NormalStrength, 0.0f, 8.0f) && mEditingMaterial->SetNormalStrength(NormalStrength)) {
                mMaterialDirty = true;
            }

            ImGui::EndDisabled();

            if (!mMaterialError.empty()) {
                ImGui::TextWrapped("%s", mMaterialError.c_str());
            }
        }

        ImGui::End();

        if (!Open && ConfirmMaterialChange()) {
            mEditingMaterial = nullptr;
            mMaterialPath.clear();
            mMaterialError.clear();
        }
    }

}
