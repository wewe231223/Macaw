/**
 * @file FEditorViewport.cpp
 * @brief 독립 에디터 뷰포트의 수명, ImGui 표시와 입력 전달을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorViewport.h"
#include "Engine/FSceneViewport.h"
#include "Engine/FRenderTarget.h"
#include "ViewportToolbar.h"
#include "Editor/FViewportCommandInfo.h"

#include <imgui.h>
#include <utility>

/**
 * @brief UI와 독립적인 단축키 정의를 ImGui 입력으로 연결하는 보조 함수를 제공합니다.
 */
namespace {
    /**
     * @brief 공통 명령에 정의된 단축키 문자를 ImGui 키 식별자로 변환합니다.
     * @param[in] Shortcut 영문 대문자, 공백 또는 탭으로 표현한 단축키입니다.
     * @return 대응하는 ImGui 키이며 지원하지 않는 문자나 0이면 ImGuiKey_None입니다.
     */
    ImGuiKey ShortcutKey(char Shortcut) {
        if (Shortcut >= 'A' && Shortcut <= 'Z') {
            return static_cast<ImGuiKey>(ImGuiKey_A + Shortcut - 'A');
        }
        return Shortcut == ' ' ? ImGuiKey_Space : Shortcut == '\t' ? ImGuiKey_Tab : ImGuiKey_None;
    }
}

/**
 * @brief 공유 편집 컨텍스트와 창 이름으로 자체 클라이언트를 구성합니다.
 */
FEditorViewport::FEditorViewport(FEditorContext* Context, FString Name)
	: mName{std::move(Name)},
	  mClient{Context} {
}

/**
 * @brief 자체 드래그와 소유 뷰포트 리소스를 정리합니다.
 */
FEditorViewport::~FEditorViewport() {
    Shutdown();
}

/**
 * @brief UI 초기화와 별도로 장면 뷰포트와 컬러 타깃을 준비합니다.
 */
bool FEditorViewport::Initialize(IRendererModule* Renderer) {
    if (Renderer == nullptr || mViewport != nullptr) {
        return false;
    }

    mViewport = std::make_unique<FSceneViewport>(Renderer, GetActiveClient());

    if (!mViewport->Resize(960, 640)) {
        mViewport.reset();
        return false;
    }

    return true;
}

/**
 * @brief 자체 드래그를 취소하고 장면 뷰포트를 해제합니다.
 */
void FEditorViewport::Shutdown() {
    mClient.GetGizmo().Cancel();
    mPlayInputCaptured = false;
    mImageSize = {};
    mViewport.reset();
}

/**
 * @brief 이 뷰포트의 장면 그리기를 클라이언트에 전달합니다.
 */
void FEditorViewport::Draw() {
    if (mViewport != nullptr) {
        mViewport->Draw();
    }
}

void FEditorViewport::DrawStatistics(float DeltaTime, const UWorld* World, const FRenderStats& Render) {
    if (mImageSize.x < 1.0f || mImageSize.y < 1.0f || !mStatistics.IsVisible()) {
        return;
    }

    if (ImGui::Begin(mName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs)) {
        mStatistics.Draw(DeltaTime, World, Render, mImageOrigin, mImageSize);
    }
    ImGui::End();
}

FViewportStatistics* FEditorViewport::GetStatistics() {
    return &mStatistics;
}

/**
 * @brief 창 표시와 도킹에 사용하는 이름을 반환합니다.
 */
const FString& FEditorViewport::GetName() const {
    return mName;
}

/**
 * @brief 소유한 뷰포트 클라이언트의 비소유 주소를 반환합니다.
 */
FEditorViewportClient* FEditorViewport::GetClient() {
    return &mClient;
}

FViewportClient* FEditorViewport::GetActiveClient() {
    return mPlayClient != nullptr ? mPlayClient : &mClient;
}

void FEditorViewport::SetPlayClient(FViewportClient* Client) {
    mClient.GetGizmo().Cancel();
    mPlayClient = Client;
    mPlayInputCaptured = false;
    if (mViewport != nullptr) {
        mViewport->SetClient(GetActiveClient());
    }
}

bool FEditorViewport::IsPlayInputCaptured() const {
    return mPlayInputCaptured;
}

/**
 * @brief 초기화된 장면 뷰포트의 비소유 주소를 반환합니다.
 */
FSceneViewport* FEditorViewport::GetSceneViewport() const {
    return mViewport.get();
}

/**
 * @brief 장면 뷰포트가 소유한 컬러 타깃의 비소유 주소를 반환합니다.
 */
FRenderTarget* FEditorViewport::GetRenderTarget() const {
    return mViewport != nullptr ? mViewport->GetRenderTarget() : nullptr;
}

/**
 * @brief 장면 타깃을 표시하고 뷰포트 입력 및 크기 변경을 전달합니다.
 * @note 이미지가 표시되지 않을 때에도 진행 중인 드래그의 종료 입력을 전달합니다.
 */
void FEditorViewport::DrawUI(float DeltaTime) {
    mImageSize = {};
    if (mViewport == nullptr) {
        return;
    }

    bool Submitted{};

    if (ImGui::Begin(mName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs)) {
        if (mPlayClient != nullptr) {
            ImGui::TextDisabled(mPlayInputCaptured ? "Playing | Shift+F1: Release input | Esc: Stop" : "Playing | Click viewport to control | Esc: Stop");
        } else {
            ViewportToolbar::Draw(&mClient);
        }

        const ImVec2 Available{ImGui::GetContentRegionAvail()};

        if (Available.x >= 1.0f && Available.y >= 1.0f && mViewport->Resize(static_cast<Uint32>(Available.x), static_cast<Uint32>(Available.y))) {
            FRenderTarget* Target{mViewport->GetRenderTarget()};
            ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Target->GetNativeTexture()))}, Available);

            const ImVec2 Origin{ImGui::GetItemRectMin()};
            mImageOrigin = {Origin.x, Origin.y};
            mImageSize = {Available.x, Available.y};
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
            State.mMiddlePressed = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
            State.mMiddleReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Middle);
            State.mControl = Input.KeyCtrl;

            State.mAlt = Input.KeyAlt;
            State.mWheel = Input.MouseWheel;
            State.mEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);

            if (mPlayClient != nullptr) {
                if (Input.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
                    mPlayInputCaptured = false;
                } else if (State.mHovered && State.mLeftPressed) {
                    ImGui::SetWindowFocus();
                    State.mFocused = true;
                    mPlayInputCaptured = true;
                    State.mDelta = {};
                } else if (!State.mFocused) {
                    mPlayInputCaptured = false;
                }
                State.mFocused = State.mFocused && mPlayInputCaptured;
            }

            const bool Keyboard{State.mFocused && !Input.WantTextInput && !Input.KeyCtrl && !Input.KeyAlt};
            State.mDelete = Keyboard && ImGui::IsKeyPressed(ImGuiKey_Delete, false);
            State.mFocusSelection = Keyboard && ImGui::IsKeyPressed(ImGuiKey_F, false);

            State.mForward = Keyboard && ImGui::IsKeyDown(ImGuiKey_W);
            State.mBackward = Keyboard && ImGui::IsKeyDown(ImGuiKey_S);
            State.mLeft = Keyboard && ImGui::IsKeyDown(ImGuiKey_A);
            State.mRight = Keyboard && ImGui::IsKeyDown(ImGuiKey_D);
            State.mUp = Keyboard && ImGui::IsKeyDown(ImGuiKey_E);
            State.mDown = Keyboard && ImGui::IsKeyDown(ImGuiKey_Q);
            State.mFast = Input.KeyShift;
            for (const FViewportCommandInfo& Info : FViewportCommandInfo::GetCommands()) {
                if (mPlayClient == nullptr && Info.mInput != nullptr) {
                    const ImGuiKey Key{ShortcutKey(Info.mShortcut)};
                    State.*Info.mInput = Keyboard && Key != ImGuiKey_None && ImGui::IsKeyPressed(Key, false);
                }
            }

            mViewport->Input(State, DeltaTime);
            Submitted = true;
        }
    }

    ImGui::End();

    if (!Submitted && mClient.GetGizmo().IsDragging() && ((mClient.GetGizmo().IsPivotDrag() ? !ImGui::IsMouseDown(ImGuiMouseButton_Middle) : !ImGui::IsMouseDown(ImGuiMouseButton_Left)) || ImGui::IsKeyPressed(ImGuiKey_Escape))) {
        FViewportInput State{};
        State.mLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        State.mMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        State.mEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);
        mViewport->Input(State, DeltaTime);
    }
}
