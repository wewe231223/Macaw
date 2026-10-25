/**
 * @file FEditorUI.cpp
 * @brief 편집 상태를 소유하지 않는 ImGui 창, 패널과 뷰포트 호스트를 구현합니다.
 * @ingroup Editor
 */

#include "FEditorUI.h"
#include "EditorStyle.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include "Engine/FProject.h"
#include "Engine/FAssetFile.h"
#include "Engine/UTexture2D.h"
#include "Engine/SceneSerializer.h"
#include "Engine/FWorldContext.h"
#include "Editor/FAssetDragDrop.h"

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
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 파일시스템 경로를 UI에 표시할 UTF-8 문자열로 변환합니다.
     * @param[in] Path 읽거나 쓸 파일 경로입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FString PathText(const std::filesystem::path& Path) {
        const auto Text{Path.u8string()};
        return FString{reinterpret_cast<const char*>(Text.data()), Text.size()};
    }
}

/**
 * @brief 편집 컨텍스트를 패널에 연결하고 독립 뷰포트의 배치 연결을 구성합니다.
 * @param[in] PlaySession 실행 제어와 조회에 사용할 PIE 세션의 비소유 주소입니다.
 */
FEditorUI::FEditorUI(FEditorContext* Context, FEditorViewport* Viewport, FPlayInEditorSession* PlaySession)
	: mContext{Context},
	  mViewport{Viewport},
	  mPlaySession{PlaySession},
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
    mIniPath = PathText(Layout);
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

    mWin32Ready = ImGui_ImplWin32_Init(Platform->GetWindowHandle());
    mDxReady = mWin32Ready && ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(Renderer->GetNativeDevice()), static_cast<ID3D11DeviceContext*>(Renderer->GetNativeContext()));

    if (!mDxReady) {
        Shutdown();

        return false;
    }

    Platform->SetMessageHandler([](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
        return ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam) != 0;
    });

    mInitialized = true;
    mExit = false;
    mRefreshContent = true;
    return true;
}

/**
 * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
 */
void FEditorUI::Shutdown() {
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
    mPlatform = nullptr;

    mExit = true;
}

/**
 * @brief 프레임 입력과 패널 UI를 구성합니다.
 */
bool FEditorUI::BeginFrame(float DeltaTime) {
    if (!mInitialized) {
        return false;
    }

    mContext->GetRenderer()->BeginFrame(mPlatform->GetWidth(), mPlatform->GetHeight());

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawMenu();
    DrawEditorToolbar();
    BuildLayout();
    FEditorContext* Inspection{mPlaySession->IsActive() ? mPlaySession->GetInspectionContext() : mContext};
    mOutlinerPanel.SetEditorContext(Inspection);
    mDetailsPanel.SetEditorContext(Inspection);
    mOutlinerPanel.Draw(DeltaTime);
    mDetailsPanel.Draw(DeltaTime);
    mViewport->DrawUI(DeltaTime);
    mConsolePanel.Draw(DeltaTime);
    DrawContent();

    return true;
}

/**
 * @brief 동기화한 장면과 통계를 그린 뒤 ImGui 드로 데이터를 제출합니다.
 */
void FEditorUI::EndFrame(float DeltaTime) {
    if (!mInitialized) {
        return;
    }

    mViewport->Draw();

    const FEditorContext* Inspection{mPlaySession->IsActive() ? mPlaySession->GetInspectionContext() : mContext};
    mViewport->DrawStatistics(DeltaTime, Inspection->GetWorld(), mContext->GetRenderer()->GetStats());

    ImGui::Render();
    mContext->GetRenderer()->BeginUiRender();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/**
 * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
 */
bool FEditorUI::WantsExit() const {
    return mExit;
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

        ImGui::DockBuilderDockWindow(mOutlinerPanel.GetName().c_str(), Left);
        ImGui::DockBuilderDockWindow(mDetailsPanel.GetName().c_str(), Right);
        ImGui::DockBuilderDockWindow(mConsolePanel.GetName().c_str(), Bottom);
        ImGui::DockBuilderDockWindow("Content", Bottom);
        ImGui::DockBuilderDockWindow(mViewport->GetName().c_str(), Center);

        ImGui::DockBuilderFinish(Dockspace);
    }
}

void FEditorUI::DrawEditorToolbar() {
    EditorStyle::PushViewportToolbar();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8.0f, 4.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.10f, 0.10f, 0.10f, 1.0f});
    const float Height{ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f};
    const ImGuiWindowFlags Flags{ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing};

    if (ImGui::BeginViewportSideBar("##EditorToolbar", ImGui::GetMainViewport(), ImGuiDir_Up, Height, Flags)) {
        DrawPlayControls();
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s%s%s", mPlaySession->IsActive() ? mPlaySession->IsPaused() ? "[Paused] " : "[Playing] " : "", mContext->GetScenePath().empty() ? "Untitled" : PathText(mContext->GetScenePath().filename()).c_str(), mContext->IsSceneDirty() ? " *" : "");
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
    const bool CanStart{!Active && mPlaySession->CanStart()};
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
            if (ImGui::MenuItem(mConsolePanel.GetName().c_str(), nullptr, mConsolePanel.IsVisible())) {
                mConsolePanel.SetVisible(!mConsolePanel.IsVisible());
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    const ImGuiIO& Input{ImGui::GetIO()};

    if (!Input.WantTextInput && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) {
        if (Input.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_P, false)) {
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
    mContext->AddLog(Imported ? "Imported " + PathText(Target) : Error.empty() ? "Cannot choose the import destination" : Error);
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
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            mRefreshContent = true;
        }
        if (mRefreshContent) {
            RefreshContent();
        }
        ImGui::Separator();
        DrawContentBreadcrumbs();
        if (!mContentError.empty()) {
            ImGui::TextWrapped("%s", mContentError.c_str());
        }
        const float PreviewHeight{mPreviewTexture != nullptr && mPreviewTexture->HasResources() ? 280.0f : 0.0f};
        const float Height{std::max(64.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - PreviewHeight)};
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
        ImGui::TextDisabled("Double-click folders to browse. Drag meshes into the viewport to place.");
        if (mPreviewTexture != nullptr && mPreviewTexture->HasResources()) {
            ImGui::Separator();
            if (ImGui::SmallButton("Close Preview")) {
                mPreviewTexture = nullptr;
            } else {
                const float SizeX{static_cast<float>(mPreviewTexture->GetSizeX())};
                const float SizeY{static_cast<float>(mPreviewTexture->GetSizeY())};
                const float Scale{std::min({std::max(1.0f, ImGui::GetContentRegionAvail().x) / SizeX, 256.0f / SizeX, 256.0f / SizeY})};
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mPreviewTexture->GetShaderResourceView()))}, ImVec2{SizeX * Scale, SizeY * Scale});
            }
        }
    }
    ImGui::End();
}

void FEditorUI::RefreshContent() {
    mAssetPaths.clear();
    mAssetTypes.clear();
    mContentFolders.clear();
    mContentFolders[{}] = {};
    mContentError.clear();
    FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    const std::filesystem::path& Root{Assets->GetContentRoot()};
    std::error_code Code{};
    std::filesystem::recursive_directory_iterator Iterator{Root, Code};
    const std::filesystem::recursive_directory_iterator End{};
    while (!Code && Iterator != End) {
        const bool Directory{Iterator->is_directory(Code)};
        const bool Link{!Code && Iterator->is_symlink(Code)};
        FString Error{};
        const auto Resolved{!Code && !Link ? Assets->ResolvePath(Iterator->path(), Error) : std::filesystem::path{}};
        if (Resolved.empty()) {
            Iterator.disable_recursion_pending();
        } else {
            const std::filesystem::path Relative{Iterator->path().lexically_relative(Root)};
            if (Directory) {
                mContentFolders[Relative.parent_path()].push_back(Relative);
                mContentFolders.try_emplace(Relative);
            } else if (Iterator->is_regular_file(Code)) {
                mAssetPaths.push_back(Relative);
                if (Relative.extension() == L".masset") {
                    EAssetType Type{};
                    if (FAssetFile::GetType(Resolved, Type, Error)) {
                        mAssetTypes.emplace(Relative, Type);
                    } else if (mContentError.empty()) {
                        mContentError = PathText(Relative) + ": " + Error;
                    }
                }
            }
        }
        if (!Code) {
            Iterator.increment(Code);
        }
    }
    if (Code) {
        mContentError = "Cannot read Content: " + Code.message();
    }
    std::sort(mAssetPaths.begin(), mAssetPaths.end());
    for (auto& Entry : mContentFolders) {
        std::sort(Entry.second.begin(), Entry.second.end());
    }
    while (!mContentDirectory.empty() && !mContentFolders.contains(mContentDirectory)) {
        mContentDirectory = mContentDirectory.parent_path();
    }
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
    const FString Label{Directory.empty() ? "Content" : PathText(Directory.filename())};
    const FString Id{PathText(Directory)};
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
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
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
        const FString Label{PathText(Part)};
        const float Right{ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x};
        if (Right - ImGui::GetItemRectMax().x > ImGui::CalcTextSize(Label.c_str()).x + 48.0f) {
            ImGui::SameLine();
        }
        ImGui::TextUnformatted("/");
        ImGui::SameLine();
        const FString Id{PathText(Path)};
        ImGui::PushID(Id.c_str());
        if (ImGui::SmallButton(Label.c_str())) {
            NavigateContent(Path);
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

void FEditorUI::DrawContentEntries() {
    if (!ImGui::BeginTable("Entries", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        return;
    }
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableHeadersRow();
    bool Empty{true};
    bool Navigate{};
    std::filesystem::path Requested{};
    const auto Folder{mContentFolders.find(mContentDirectory)};
    if (Folder != mContentFolders.end()) {
        for (const auto& Path : Folder->second) {
            Empty = false;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const FString Label{"[Folder] " + PathText(Path.filename())};
            if (ImGui::Selectable(Label.c_str(), mSelectedContentPath == Path, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
                mSelectedContentPath = Path;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    Requested = Path;
                    Navigate = true;
                }
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled("Folder");
        }
    }
    for (const auto& Path : mAssetPaths) {
        if (Path.parent_path() != mContentDirectory) {
            continue;
        }
        Empty = false;
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        const FString Label{PathText(Path.filename())};
        const auto Type{mAssetTypes.find(Path)};
        if (ImGui::Selectable(Label.c_str(), mSelectedContentPath == Path, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
            mSelectedContentPath = Path;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && Type != mAssetTypes.end() && mContext->CanEditWorld()) {
                OpenContentAsset(Path, Type->second);
            }
        }
        if (Type != mAssetTypes.end() && Type->second == EAssetType::Mesh && mContext->CanEditWorld() && ImGui::BeginDragDropSource()) {
            const FString Relative{PathText(Path)};
            ImGui::SetDragDropPayload(EditorAssetDragDrop::MeshPayload, Relative.c_str(), Relative.size() + 1);
            ImGui::TextUnformatted(Label.c_str());
            ImGui::TextDisabled("Drop into the viewport to place");
            ImGui::EndDragDropSource();
        }
        ImGui::TableSetColumnIndex(1);
        const char* Kind{Type != mAssetTypes.end() ? Type->second == EAssetType::Mesh ? "Mesh" : Type->second == EAssetType::Scene ? "Scene" : "Texture" : Path.extension() == L".hlsl" || Path.extension() == L".hlsli" ? "Shader" : Path.extension() == L".lua" ? "Script" : Path.extension() == L".masset" ? "Invalid Asset" : "File"};
        ImGui::TextDisabled("%s", Kind);
    }
    ImGui::EndTable();
    if (Empty) {
        ImGui::TextDisabled("This folder is empty.");
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
        mContext->SpawnMesh(Path);
    } else if (Type == EAssetType::Texture) {
        FString Error{};
        mPreviewTexture = mContext->GetProject()->GetAssets()->LoadTexture(Path, Error);
        if (mPreviewTexture != nullptr && mPreviewTexture->CreateResources(static_cast<ID3D11Device*>(mContext->GetRenderer()->GetNativeDevice()))) {
            mContext->AddLog("Loaded " + PathText(Path));
        } else {
            mContext->AddLog(Error.empty() ? "Cannot preview the texture" : Error);
        }
    }
}
