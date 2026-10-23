/**
 * @file FEditorUI.cpp
 * @brief 편집 상태를 소유하지 않는 ImGui 창, 패널과 뷰포트 호스트를 구현합니다.
 * @ingroup Editor
 */

#include "FEditorUI.h"
#include "ViewportToolbar.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewportClient.h"
#include "Engine/FSceneViewport.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FViewportInput.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"

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
 * @brief 편집 컨텍스트와 뷰포트 클라이언트를 패널에 연결합니다.
 */
FEditorUI::FEditorUI(FEditorContext* Context, FEditorViewportClient* Client)
	: mContext{Context},
	  mClient{Client},
	  mOutlinerPanel{Context},
	  mDetailsPanel{Context},
	  mConsolePanel{Context},
	  mStatisticsPanel{Context} {
}

/**
 * @brief UI 백엔드와 표시 리소스를 정리합니다.
 */
FEditorUI::~FEditorUI() {
    Shutdown();
}

/**
 * @brief ImGui 백엔드와 장면 표시용 뷰포트를 준비합니다.
 */
bool FEditorUI::Initialize(IRendererModule* Renderer, FWindowsApplication* Platform) {
    if (Renderer == nullptr || Platform == nullptr || mContextCreated) {
        return false;
    }

    mPlatform = Platform;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    mContextCreated = true;

    ImGuiIO& Input{ImGui::GetIO()};
    Input.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

    mIniPath = "EditorLayout.ini";
    Input.IniFilename = mIniPath.c_str();
    Input.LogFilename = nullptr;

    if (std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf")) {
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

    mViewport = std::make_unique<FSceneViewport>(Renderer, mClient);

    if (!mViewport->Resize(960, 640)) {
        Shutdown();

        return false;
    }

    mInitialized = true;
    mExit = false;
    return true;
}

/**
 * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
 */
void FEditorUI::Shutdown() {
    if (mPlatform != nullptr) {
        mPlatform->SetMessageHandler({});
    }

    mViewport.reset();

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

    mDeltaTime = DeltaTime;
    mContext->GetRenderer()->BeginFrame(mPlatform->GetWidth(), mPlatform->GetHeight());

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawMenu();
    BuildLayout();
    mOutlinerPanel.Draw(DeltaTime);
    mDetailsPanel.Draw(DeltaTime);
    DrawViewport();
    mConsolePanel.Draw(DeltaTime);

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

    mStatisticsPanel.Draw(DeltaTime);

    ImGui::Render();
    mContext->GetRenderer()->BeginUiRender();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/**
 * @brief 에디터 뷰포트 렌더 타깃의 비소유 주소를 반환합니다.
 */
FRenderTarget* FEditorUI::GetViewportTarget() const {
    return mViewport != nullptr ? mViewport->GetRenderTarget() : nullptr;
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
        const std::wstring Filename{mContext->GetScenePath().empty() ? L"Scene.json" : mContext->GetScenePath().filename().wstring()};
        std::copy_n(Filename.begin(), std::min(Filename.size(), Buffer.size() - 1), Buffer.begin());
    }

    const std::filesystem::path Directory{std::filesystem::absolute("Scenes")};

    std::error_code Error{};
    std::filesystem::create_directories(Directory, Error);

    OPENFILENAMEW Dialog{};
    Dialog.lStructSize = sizeof(Dialog);
    Dialog.hwndOwner = mPlatform->GetWindowHandle();
    Dialog.lpstrFilter = L"Scene JSON (*.json)\0*.json\0All files (*.*)\0*.*\0";
    Dialog.lpstrFile = Buffer.data();
    Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
    Dialog.lpstrInitialDir = Directory.c_str();
    Dialog.lpstrDefExt = L"json";
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

        ImGuiID Outliner{Left};
        const ImGuiID Statistics{ImGui::DockBuilderSplitNode(Outliner, ImGuiDir_Down, 0.42f, nullptr, &Outliner)};

        ImGui::DockBuilderDockWindow(mOutlinerPanel.GetName().c_str(), Outliner);
        ImGui::DockBuilderDockWindow(mStatisticsPanel.GetName().c_str(), Statistics);
        ImGui::DockBuilderDockWindow(mDetailsPanel.GetName().c_str(), Right);
        ImGui::DockBuilderDockWindow(mConsolePanel.GetName().c_str(), Bottom);
        ImGui::DockBuilderDockWindow("Viewport", Center);

        ImGui::DockBuilderFinish(Dockspace);
    }
}

/**
 * @brief 장면 파일과 패널 표시 메뉴를 그립니다.
 */
void FEditorUI::DrawMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                mContext->NewScene();
            }

            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                const auto Path{ChooseFile(false)};

                if (!Path.empty()) {
                    mContext->LoadScene(Path);
                }
            }

            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                const auto Path{mContext->GetScenePath().empty() ? ChooseFile(true) : mContext->GetScenePath()};

                if (!Path.empty()) {
                    mContext->SaveScene(Path);
                }
            }

            if (ImGui::MenuItem("Save Scene As...")) {
                const auto Path{ChooseFile(true)};

                if (!Path.empty()) {
                    mContext->SaveScene(Path);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                mExit = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem(mConsolePanel.GetName().c_str(), nullptr, mConsolePanel.IsVisible())) {
                mConsolePanel.SetVisible(!mConsolePanel.IsVisible());
            }

            if (ImGui::MenuItem(mStatisticsPanel.GetName().c_str(), nullptr, mStatisticsPanel.IsVisible())) {
                mStatisticsPanel.SetVisible(!mStatisticsPanel.IsVisible());
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::TextDisabled("%s%s", mContext->GetScenePath().empty() ? "Untitled" : PathText(mContext->GetScenePath().filename()).c_str(), mContext->IsSceneDirty() ? " *" : "");
        ImGui::EndMainMenuBar();
    }

    const ImGuiIO& Input{ImGui::GetIO()};

    if (Input.KeyCtrl && !Input.WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            const auto Path{mContext->GetScenePath().empty() ? ChooseFile(true) : mContext->GetScenePath()};

            if (!Path.empty()) {
                mContext->SaveScene(Path);
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_O)) {
            const auto Path{ChooseFile(false)};

            if (!Path.empty()) {
                mContext->LoadScene(Path);
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_N)) {
            mContext->NewScene();
        }
    }
}

/**
 * @brief 장면 타깃을 표시하고 뷰포트 입력 및 크기 변경을 전달합니다.
 */
void FEditorUI::DrawViewport() {
    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs)) {
        ViewportToolbar::Draw(mClient);

        const ImVec2 Available{ImGui::GetContentRegionAvail()};

        if (Available.x >= 1.0f && Available.y >= 1.0f && mViewport->Resize(static_cast<Uint32>(Available.x), static_cast<Uint32>(Available.y))) {
            FRenderTarget* Target{mViewport->GetRenderTarget()};
            ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Target->GetNativeTexture()))}, Available);

            const ImVec2 Origin{ImGui::GetItemRectMin()};
            const ImGuiIO& Input{ImGui::GetIO()};
            FViewportInput State{};
            const float ScaleX{Target->GetWidth() / Available.x};
            const float ScaleY{Target->GetHeight() / Available.y};

            State.mMouse = {(Input.MousePos.x - Origin.x) * ScaleX, (Input.MousePos.y - Origin.y) * ScaleY};
            State.mDelta = {Input.MouseDelta.x * ScaleX, Input.MouseDelta.y * ScaleY};

            State.mHovered = ImGui::IsItemHovered();
            State.mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);

            State.mLeftPressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            State.mLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            State.mLeftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
            State.mRightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            State.mMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

            State.mAlt = Input.KeyAlt;
            State.mWheel = Input.MouseWheel;
            State.mEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);

            const bool Keyboard{State.mFocused && !Input.WantTextInput && !Input.KeyCtrl && !Input.KeyAlt};
            State.mDelete = Keyboard && ImGui::IsKeyPressed(ImGuiKey_Delete, false);
            State.mCycleGizmo = Keyboard && ImGui::IsKeyPressed(ImGuiKey_Space, false);
            State.mFocusSelection = Keyboard && ImGui::IsKeyPressed(ImGuiKey_F, false);

            State.mForward = Keyboard && ImGui::IsKeyDown(ImGuiKey_W);
            State.mBackward = Keyboard && ImGui::IsKeyDown(ImGuiKey_S);
            State.mLeft = Keyboard && ImGui::IsKeyDown(ImGuiKey_A);
            State.mRight = Keyboard && ImGui::IsKeyDown(ImGuiKey_D);
            State.mUp = Keyboard && ImGui::IsKeyDown(ImGuiKey_E);
            State.mDown = Keyboard && ImGui::IsKeyDown(ImGuiKey_Q);
            State.mFast = Input.KeyShift;

            mViewport->Input(State, mDeltaTime);
        }
    }

    ImGui::End();
}
