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

#include <d3d11.h>
#include <commdlg.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <algorithm>
#include <array>

/**
 * @brief ImGui Win32 백엔드의 외부 메시지 처리 진입점입니다.
 * @param[in] Window 메시지를 받은 Win32 창입니다.
 * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
 * @param[in] WParam Windows 메시지의 WPARAM 인수입니다.
 * @param[in] LParam Windows 메시지의 LPARAM 인수입니다.
 * @return 연산을 적용한 새 값입니다.
 */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

/**
 * @brief 편집 컨텍스트를 패널에 연결하고 독립 뷰포트의 배치 연결을 구성합니다.
 * @param[in] PlaySession 실행 제어와 조회에 사용할 PIE 세션의 비소유 주소입니다.
 */
FEditorUI::FEditorUI(FEditorContext* Context, FEditorViewport* Viewport, FPlayInEditorSession* PlaySession)
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
FEditorUI::~FEditorUI() {
    Shutdown();
}

/**
 * @brief 독립 뷰포트를 배치할 ImGui 백엔드를 준비합니다.
 */
bool FEditorUI::Initialize(IRendererModule* Renderer, FWindowsApplication* Platform) {
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
    mIniPath = FilePath::ToUtf8(Layout);
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

    Platform->SetMessageHandler([](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
        return ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam) != 0;
    });

    mInitialized = true;
    InstallEditorIcons();
    if (mContext->GetWorld() != nullptr && mContext->GetWorld()->GetPersistentLevel() != nullptr) {
        for (const auto& Actor : mContext->GetWorld()->GetPersistentLevel()->GetActors()) {
            for (const auto& Component : Actor->GetComponents()) {
                if (auto* Light{Cast<ULightComponentBase>(Component.get())}) {
                    Light->SynchronizeLight();
                }
            }
        }
    }
    mAssetCatalog = std::make_unique<FEditorAssetCatalog>(mContext->GetProject()->GetAssets());
    mThumbnails = std::make_unique<FAssetThumbnailCache>(mContext->GetProject()->GetAssets(), Renderer);
    mDetailsPanel.SetAssetServices(mAssetCatalog.get(), mThumbnails.get());
    mMeshViewer = std::make_unique<FMeshViewer>(Renderer, mContext->GetProject()->GetAssets());
    mExit = false;
    mRefreshContent = true;
    RefreshContent();
    return true;
}

void FEditorUI::InstallEditorIcons() {
    std::array<wchar_t, 32768> Executable{};
    const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
    const std::filesystem::path Bundled{Length > 0 && Length < Executable.size() ? std::filesystem::path{Executable.data()}.parent_path() / "Templates/Project/Content/Editor/Icon" : std::filesystem::path{}};
    FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    for (const char* Name : {"SpotLight_64x.png", "PointLight_64x.png", "DirectionalLight_64x.png", "SkyLight_64x.png", "Pawn_64x.png", "Sources.txt"}) {
        FString Error{};
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
            mContext->AddLog("Cannot find bundled editor icon: " + FString{Name});
            continue;
        }
        std::filesystem::create_directories(Target.parent_path(), Code);
        if (!Code) {
            std::filesystem::copy_file(Source, Target, std::filesystem::copy_options::skip_existing, Code);
        }
        if (Code) {
            mContext->AddLog("Cannot install editor icon '" + FString{Name} + "': " + Code.message());
        }
    }
}

/**
 * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
 */
void FEditorUI::Shutdown() {
    mMeshViewer.reset();
    mDetailsPanel.SetAssetServices(nullptr, nullptr);
    mThumbnails.reset();
    mAssetCatalog.reset();
    mAssetDisplayPaths.clear();
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

    mExit = true;
}

/**
 * @brief 프레임 입력과 패널 UI를 구성합니다.
 */
bool FEditorUI::BeginFrame(float DeltaTime) {
    static FStat UiTime{"unit", "Editor UI CPU", EStatUnit::Milliseconds};
    const FScopedStatTimer StatTimer{UiTime};
    if (!mInitialized) {
        return false;
    }

    mContext->GetRenderer()->BeginFrame(mPlatform->GetWidth(), mPlatform->GetHeight());

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    mUIScale.Update(*mPlatform);
    ImGui::NewFrame();
    mThumbnails->BeginFrame();
    if (mRefreshContent) {
        RefreshContent();
    }
    if (mEditingMaterial != nullptr) {
        mThumbnails->GetMaterialPreview(mEditingMaterial);
    }

    DrawMenu();
    DrawEditorToolbar();
    BuildLayout();
    FEditorContext* Inspection{mPlaySession->IsActive() ? mPlaySession->GetInspectionContext() : mContext};
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
    if (mMeshViewer != nullptr) {
        mMeshViewer->DrawUI();
    }

    return true;
}

/**
 * @brief 동기화한 장면과 통계를 그린 뒤 ImGui 드로 데이터를 제출합니다.
 */
void FEditorUI::EndFrame(float DeltaTime) {
    if (!mInitialized) {
        return;
    }

    mViewportLayout.Draw(DeltaTime);
    DrawShadowAtlas();

    static FStat UiSubmitTime{"unit", "ImGui submission CPU", EStatUnit::Milliseconds};
    const FScopedStatTimer StatTimer{UiSubmitTime};
    ImGui::Render();
    mContext->GetRenderer()->BeginUiRender();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void FEditorUI::DrawShadowAtlas() {
    if (!mContext->IsShadowAtlasOpen()) {
        return;
    }
    bool Open{true};
    ImGui::SetNextWindowSize({760.0f, 840.0f}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Shadow Atlas", &Open)) {
        const FShadowAtlasView Atlas{mContext->GetRenderer()->GetShadowAtlasView()};
        const FRenderStats Stats{mContext->GetRenderer()->GetStats()};
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
                for (const FShadowAtlasTile& Tile : Atlas.mTiles) {
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
bool FEditorUI::WantsExit() const {
    return mExit;
}

FEditorViewport* FEditorUI::GetActiveViewport() const {
    return mViewportLayout.GetActiveViewport();
}

/**
 * @brief 장면 로드 또는 저장용 Windows 파일 대화상자를 표시합니다.
 */
std::filesystem::path FEditorUI::ChooseFile(bool Save) {
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
void FEditorUI::BuildLayout() {
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

void FEditorUI::DrawEditorToolbar() {
    EditorStyle::PushViewportToolbar();
    const float Scale{mUIScale.GetScale()};
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8.0f * Scale, 4.0f * Scale});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.10f, 0.10f, 0.10f, 1.0f});
    const float Height{ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f};
    const ImGuiWindowFlags Flags{ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing};

    if (ImGui::BeginViewportSideBar("##EditorToolbar", ImGui::GetMainViewport(), ImGuiDir_Up, Height, Flags)) {
        DrawPlayControls();
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s%s%s", mPlaySession->IsActive() ? mPlaySession->IsPaused() ? "[Paused] " : "[Playing] " : "", mContext->GetScenePath().empty() ? "Untitled" : FilePath::ToUtf8(mContext->GetScenePath().filename()).c_str(), mContext->IsSceneDirty() ? " *" : "");
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    EditorStyle::PopViewportToolbar();
}

void FEditorUI::DrawPlayControls() {
    ImGui::PushID("PIEControls");
    const bool Active{mPlaySession->IsActive()};
    const bool Paused{mPlaySession->IsPaused()};
    const bool CanStart{!Active && mViewportLayout.CanChangeLayout() && mPlaySession->CanStart()};
    const char* PlayTooltip{Active ? "Stop (Esc)" : CanStart ? "Play (Alt+P)" : "Play requires an active world camera and an idle editor"};

    if (EditorStyle::IconButton("PlayStop", Active ? EditorStyle::EIcon::Stop : EditorStyle::EIcon::Play, false, Active || CanStart, false, PlayTooltip)) {
        if (Active) {
            mPlaySession->Stop();
        } else {
            mPlaySession->Start();
        }
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorStyle::IconButton("PauseResume", Paused ? EditorStyle::EIcon::Play : EditorStyle::EIcon::Pause, Paused, Active, false, Paused ? "Resume" : "Pause")) {
        mPlaySession->SetPaused(!Paused);
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (EditorStyle::IconButton("Step", EditorStyle::EIcon::Step, false, Active && Paused, false, "Advance 1 Tick")) {
        mPlaySession->Step();
    }
    ImGui::PopID();
}

/**
 * @brief 장면 파일과 패널 표시 메뉴를 그립니다.
 */
void FEditorUI::DrawMenu() {
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
                FString Error{};
                if (!mContext->SaveScene(mContext->GetScenePath()) || !mContext->GetProject()->SetStartupScene(mContext->GetScenePath(), Error)) {
                    mContext->AddLog(Error);
                }
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
                FViewportStatistics* Statistics{GetActiveViewport()->GetStatistics()};
                for (const FString& Group : FStats::Get().GetGroups()) {
                    if (ImGui::MenuItem(Group.c_str(), nullptr, Statistics->IsEnabled(Group))) {
                        FString Output{};
                        Statistics->ExecuteCommand("stat " + Group, Output);
                    }
                }
                ImGui::EndMenu();
            }
            mUIScale.DrawMenu();
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

        ImGui::EndMainMenuBar();
    }

    const ImGuiIO& Input{ImGui::GetIO()};

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

bool FEditorUI::ConfirmSceneChange() {
    if (!ConfirmMaterialChange()) {
        return false;
    }
    if (!mContext->IsSceneDirty()) {
        return true;
    }
    const Int32 Choice{MessageBoxW(mPlatform->GetWindowHandle(), L"Save changes to the current scene?", L"Scene", MB_YESNOCANCEL | MB_ICONQUESTION)};
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

void FEditorUI::ChooseProject(bool Create) {
    if (!ConfirmSceneChange()) {
        return;
    }
    mContext->RequestProject({}, Create);
}

void FEditorUI::ImportAsset(bool Texture) {
    std::array<wchar_t, 32768> Buffer{};
    OPENFILENAMEW Dialog{};
    Dialog.lStructSize = sizeof(Dialog);
    Dialog.hwndOwner = mPlatform->GetWindowHandle();
    Dialog.lpstrTitle = Texture ? L"Import Texture" : L"Import Mesh";
    Dialog.lpstrFilter = Texture ? L"Images\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds;*.hdr\0" : L"Meshes\0*.obj;*.fbx;*.gltf;*.glb\0";
    Dialog.lpstrFile = Buffer.data();
    Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
    Dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST;
    if (!GetOpenFileNameW(&Dialog)) {
        return;
    }
    const std::filesystem::path Source{Buffer.data()};
    FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    const std::filesystem::path Folder{mContentDirectory};
    std::filesystem::path Target{Folder / Source.stem()};
    Target += L".masset";
    std::error_code Code{};
    for (Uint32 Index{1}; std::filesystem::exists(Assets->GetContentRoot() / Target, Code) && !Code; ++Index) {
        Target = Folder / (Source.stem().wstring() + L"_" + std::to_wstring(Index) + L".masset");
    }
    FString Error{};
    const bool Imported{!Code && (Texture ? Assets->ImportTexture(Source, Target, Error) : Assets->ImportMesh(Source, Target, Error))};
    mRefreshContent = true;
    mContext->AddLog(Imported ? "Imported " + FilePath::ToUtf8(Target) : Error.empty() ? "Cannot choose the import destination" : Error);
}

void FEditorUI::DrawContent() {
    if (ImGui::Begin("Content")) {
        ImGui::TextUnformatted(mContext->GetProject()->GetName().c_str());
        ImGui::BeginDisabled(!mContext->CanEditWorld());
        if (ImGui::Button("Import Mesh...")) {
            ImportAsset(false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Import Texture...")) {
            ImportAsset(true);
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
        const char* FilterName{!mContentFilter.has_value() ? "All Assets" : *mContentFilter == EAssetType::Mesh ? "Meshes" : *mContentFilter == EAssetType::Material ? "Materials" : *mContentFilter == EAssetType::Texture ? "Textures" : "Scenes"};
        if (ImGui::BeginCombo("##AssetType", FilterName)) {
            if (ImGui::Selectable("All Assets", !mContentFilter.has_value())) {
                mContentFilter.reset();
            }
            for (EAssetType Type : {EAssetType::Mesh, EAssetType::Material, EAssetType::Texture, EAssetType::Scene}) {
                const char* Name{Type == EAssetType::Mesh ? "Meshes" : Type == EAssetType::Material ? "Materials" : Type == EAssetType::Texture ? "Textures" : "Scenes"};
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
        ImGui::TextDisabled("Double-click to open. Drag a thumbnail onto Details to assign.");
    }
    ImGui::End();
}

void FEditorUI::DrawTexturePreview() {
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
        ImGui::TextUnformatted(mAssetCatalog->GetDisplayPath(mPreviewTexturePath).c_str());
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

void FEditorUI::RefreshContent() {
    mAssetPaths.clear();
    mAssetTypes.clear();
    mAssetDisplayPaths.clear();
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
        mAssetDisplayPaths.emplace(Entry.GetPath(), Entry.GetDisplayPath());
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

void FEditorUI::NavigateContent(const std::filesystem::path& Directory) {
    if (mContentFolders.contains(Directory)) {
        mContentDirectory = Directory;
        mSelectedContentPath.clear();
        mRevealContentDirectory = true;
    }
}

void FEditorUI::DrawContentTree(const std::filesystem::path& Directory) {
    const auto Entry{mContentFolders.find(Directory)};
    if (Entry == mContentFolders.end()) {
        return;
    }
    const FString Label{Directory.empty() ? "Content" : FilePath::ToUtf8(Directory.filename())};
    const FString Id{FilePath::ToUtf8(Directory)};
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

void FEditorUI::DrawContentBreadcrumbs() {
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
        const FString Label{FilePath::ToUtf8(Part)};
        const float Right{ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x};
        if (Right - ImGui::GetItemRectMax().x > ImGui::CalcTextSize(Label.c_str()).x + 48.0f) {
            ImGui::SameLine();
        }
        ImGui::TextUnformatted("/");
        ImGui::SameLine();
        const FString Id{FilePath::ToUtf8(Path)};
        ImGui::PushID(Id.c_str());
        if (ImGui::SmallButton(Label.c_str())) {
            NavigateContent(Path);
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

void FEditorUI::DrawContentEntries() {
    const ImGuiTextFilter Search{mContentSearch.data()};
    const bool Recursive{mContentFilter.has_value() || mContentSearch[0] != '\0'};
    TArray<std::filesystem::path> Folders{};
    TArray<std::filesystem::path> Assets{};
    const auto Folder{mContentFolders.find(mContentDirectory)};
    if (!Recursive && Folder != mContentFolders.end()) {
        Folders = Folder->second;
    }
    for (const auto& Path : mAssetPaths) {
        const auto& Display{mAssetDisplayPaths.at(Path)};
        const auto Type{mAssetTypes.find(Path)};
        const auto Relative{Display.lexically_relative(mContentDirectory)};
        const bool InFolder{Recursive ? mContentDirectory.empty() || (!Relative.empty() && *Relative.begin() != L"..") : Display.parent_path() == mContentDirectory};
        if (!InFolder || (mContentFilter.has_value() && (Type == mAssetTypes.end() || Type->second != *mContentFilter)) || !Search.PassFilter(FilePath::ToGenericUtf8(Display).c_str())) {
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
                    const auto& Display{Directory ? Path : mAssetDisplayPaths.at(Path)};
                    const auto Type{mAssetTypes.find(Path)};
                    const bool HasThumbnail{!Directory && Type != mAssetTypes.end() && (Type->second == EAssetType::Mesh || Type->second == EAssetType::Material || Type->second == EAssetType::Texture)};
                    void* Thumbnail{HasThumbnail ? mThumbnails->GetThumbnail(Path, Type->second) : nullptr};
                    const char* Kind{Directory ? "Folder" : Type == mAssetTypes.end() ? "File" : Type->second == EAssetType::Mesh ? "Mesh" : Type->second == EAssetType::Material ? "Material" : Type->second == EAssetType::Texture ? "Texture" : "Scene"};
                    const FString Actual{FilePath::ToGenericUtf8(Path)};
                    const FString Name{FilePath::ToUtf8(Directory ? Display.filename() : Display.stem())};
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
                    if (HasThumbnail && mContext->CanEditWorld() && ImGui::BeginDragDropSource()) {
                        const char* Payload{Type->second == EAssetType::Mesh ? EditorAssetDragDrop::MeshPayload : Type->second == EAssetType::Material ? EditorAssetDragDrop::MaterialPayload : EditorAssetDragDrop::TexturePayload};
                        ImGui::SetDragDropPayload(Payload, Actual.c_str(), Actual.size() + 1);
                        if (Thumbnail != nullptr) {
                            ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Thumbnail))}, {64.0f * Scale, 64.0f * Scale});
                        }
                        ImGui::TextUnformatted(Name.c_str());
                        ImGui::EndDragDropSource();
                    }
                    if (Hovered) {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(FilePath::ToGenericUtf8(Display).c_str());
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

void FEditorUI::OpenContentAsset(const std::filesystem::path& Path, EAssetType Type) {
    if (Type == EAssetType::Scene) {
        if (ConfirmSceneChange()) {
            mContext->LoadScene(Path);
        }
    } else if (Type == EAssetType::Mesh) {
        FString Error{};
        if (mMeshViewer != nullptr && !mMeshViewer->Open(Path, Error)) {
            mContext->AddLog(Error);
        }
    } else if (Type == EAssetType::Material) {
        if (!ConfirmMaterialChange()) {
            return;
        }
        UMaterial* Material{mContext->GetProject()->GetAssets()->LoadMaterial(Path, mMaterialError)};
        if (Material != nullptr) {
            mEditingMaterial = Material;
            mMaterialPath = Path;
            mMaterialDirty = false;
        } else {
            mContext->AddLog(mMaterialError);
        }
    } else if (Type == EAssetType::Texture) {
        FString Error{};
        UTexture2D* Texture{mContext->GetProject()->GetAssets()->LoadTexture(Path, Error)};
        if (Texture != nullptr && Texture->CreateResources(static_cast<ID3D11Device*>(mContext->GetRenderer()->GetNativeDevice()))) {
            mPreviewTexture = Texture;
            mPreviewTexturePath = Path;
            mFocusTexturePreview = true;
            mContext->AddLog("Loaded " + FilePath::ToUtf8(Path));
        } else {
            mContext->AddLog(Error.empty() ? "Cannot preview the texture" : Error);
        }
    }
}

void FEditorUI::CreateMaterial() {
    if (!ConfirmMaterialChange()) {
        return;
    }
    FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    std::filesystem::path Path{mContentDirectory / "Material.mtl"};
    std::error_code Code{};
    for (Uint32 Index{1}; std::filesystem::exists(Assets->GetContentRoot() / Path, Code) && !Code; ++Index) {
        Path = mContentDirectory / ("Material_" + std::to_string(Index) + ".mtl");
    }
    auto Material{NewObject<UMaterial>()};
    Material->SetName(FilePath::ToUtf8(Path.stem()));
    if (Code || !Assets->SaveMaterial(Path, *Material, mMaterialError)) {
        mContext->AddLog(mMaterialError.empty() ? "Cannot create the material" : mMaterialError);
        return;
    }
    mRefreshContent = true;
    mSelectedContentPath = Path;
    OpenContentAsset(Path, EAssetType::Material);
}

bool FEditorUI::SaveMaterial() {
    if (mEditingMaterial == nullptr || !mContext->GetProject()->GetAssets()->SaveMaterial(mMaterialPath, *mEditingMaterial, mMaterialError)) {
        return false;
    }
    mMaterialDirty = false;
    mRefreshContent = true;
    mContext->AddLog("Saved " + FilePath::ToUtf8(mMaterialPath));
    return true;
}

bool FEditorUI::ConfirmMaterialChange() {
    if (!mMaterialDirty || mEditingMaterial == nullptr) {
        return true;
    }
    const Int32 Choice{MessageBoxW(mPlatform->GetWindowHandle(), L"Save changes to the current material?", L"Material", MB_YESNOCANCEL | MB_ICONQUESTION)};
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

bool FEditorUI::SetMaterialTexture(const FString& Path) {
    FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    if (!Path.empty() && Assets->LoadTexture(FilePath::FromUtf8(Path), mMaterialError) == nullptr) {
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

bool FEditorUI::SetMaterialShader(const FString& Path) {
    auto Candidate{NewObject<UMaterial>()};
    Candidate->SetTwoSided(mEditingMaterial->IsTwoSided());
    FRenderer* Renderer{dynamic_cast<FRenderer*>(mContext->GetRenderer())};
    if (!Candidate->SetShaderPath(Path) || Renderer == nullptr || Renderer->GetMaterialPipeline(*Candidate) == nullptr) {
        mMaterialError = "Cannot use this material shader. Check the shader diagnostics in the log.";
        return false;
    }
    mEditingMaterial->SetShaderPath(Path);
    mMaterialError.clear();
    mMaterialDirty = true;
    return true;
}

void FEditorUI::DrawMaterialEditor() {
    if (mEditingMaterial == nullptr) {
        return;
    }
    bool Open{true};
    ImGui::SetNextWindowSize({560.0f, 700.0f}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin(mMaterialDirty ? "Material Editor*###MaterialEditor" : "Material Editor###MaterialEditor", &Open)) {
        ImGui::TextUnformatted(mAssetCatalog->GetDisplayPath(mMaterialPath).c_str());
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
            TArray<FString> Shaders{"Shaders/Lit.hlsl", "Shaders/Unlit.hlsl"};
            for (const auto& Path : mAssetPaths) {
                const FString Name{FilePath::ToUtf8(Path)};
                if (Path.extension() == L".hlsl" && Path.filename() != L"Line.hlsl" && Path.filename() != L"Gizmo.hlsl" && Path.filename() != L"Text.hlsl" && std::find(Shaders.begin(), Shaders.end(), Name) == Shaders.end()) {
                    Shaders.push_back(Name);
                }
            }
            for (const FString& Shader : Shaders) {
                if (ImGui::Selectable(Shader.c_str(), mEditingMaterial->GetShaderPath() == Shader)) {
                    SetMaterialShader(Shader);
                }
            }
            ImGui::EndCombo();
        }
        std::array<char, 512> ShaderPath{};
        const FString& CurrentShader{mEditingMaterial->GetShaderPath()};
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
        FString TexturePath{mEditingMaterial->GetBaseColorTexturePath()};
        if (mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, "Base Color Texture", TexturePath, EAssetType::Texture)) {
            SetMaterialTexture(TexturePath);
        }
        FString NormalPath{mEditingMaterial->GetNormalMapTexturePath()};
        if (mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, "Normal Map", NormalPath, EAssetType::Texture)) {
            const FString Previous{mEditingMaterial->GetNormalMapTexturePath()};
            if (mEditingMaterial->SetNormalMapTexturePath(NormalPath) && mEditingMaterial->ResolveTexture(*mContext->GetProject()->GetAssets(), mMaterialError)) {
                mMaterialDirty = true;
            } else {
                mEditingMaterial->SetNormalMapTexturePath(Previous);
                FString RestoreError{};
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
