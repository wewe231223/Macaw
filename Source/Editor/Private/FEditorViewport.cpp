/**
 * @file FEditorViewport.cpp
 * @brief 독립 에디터 뷰포트의 수명, ImGui 표시와 입력 전달을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorViewport.h"
#include "Engine/FSceneViewport.h"
#include "Engine/FRenderTarget.h"
#include "ViewportToolbar.h"

#include <imgui.h>
#include <utility>

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

    mViewport = std::make_unique<FSceneViewport>(Renderer, &mClient);

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
    if (mViewport == nullptr) {
        return;
    }

    bool Submitted{};

    if (ImGui::Begin(mName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs)) {
        ViewportToolbar::Draw(&mClient);

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

            mViewport->Input(State, DeltaTime);
            Submitted = true;
        }
    }

    ImGui::End();

    if (!Submitted && mClient.GetGizmo().IsDragging() && (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_Escape))) {
        FViewportInput State{};
        State.mLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        State.mEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);
        mViewport->Input(State, DeltaTime);
    }
}
