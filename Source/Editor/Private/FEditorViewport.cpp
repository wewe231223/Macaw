/**
 * @file FEditorViewport.cpp
 * @brief 독립 에디터 뷰포트의 수명, ImGui 표시와 입력 전달을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorViewport.h"
#include "Core/FilePath.h"
#include "Engine/FSceneViewport.h"
#include "Engine/FRenderTarget.h"
#include "ViewportToolbar.h"
#include "Editor/FViewportCommandInfo.h"
#include "Editor/FAssetDragDrop.h"

#include <imgui.h>
#include <utility>

namespace Editor {

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
    Editor::FEditorViewport::FEditorViewport(Editor::FEditorContext* Context, Core::FString Name)
		: mName{std::move(Name)},
		  mClient{Context} {
    }

    /**
     * @brief 자체 드래그와 소유 뷰포트 리소스를 정리합니다.
     */
    Editor::FEditorViewport::~FEditorViewport() {
        Shutdown();
    }

    /**
     * @brief UI 초기화와 별도로 장면 뷰포트와 컬러 타깃을 준비합니다.
     */
    bool Editor::FEditorViewport::Initialize(RenderCore::IRendererModule* Renderer) {
        if (Renderer == nullptr || mViewport != nullptr) {
            return false;
        }

        mViewport = std::make_unique<Engine::FSceneViewport>(Renderer, GetActiveClient());

        if (!mViewport->Resize(960, 640)) {
            mViewport.reset();
            return false;
        }

        return true;
    }

    /**
     * @brief 자체 드래그를 취소하고 장면 뷰포트를 해제합니다.
     */
    void Editor::FEditorViewport::Shutdown() {
        mClient.GetGizmo().Cancel();
        mPlayInputCaptured = false;
        mImageSize = {};
        mDrawList = nullptr;
        mMouseCaptured = false;
        mViewport.reset();
    }

    /**
     * @brief 이 뷰포트의 장면 그리기를 클라이언트에 전달합니다.
     */
    void Editor::FEditorViewport::Draw() {
        if (mViewport != nullptr) {
            mViewport->Draw();
        }
    }

    void Editor::FEditorViewport::DrawStatistics(float DeltaTime, const Engine::UWorld* World, const RenderCore::FRenderStats& Render) {
        if (mImageSize.x < 1.0f || mImageSize.y < 1.0f || mDrawList == nullptr || !mStatistics.IsVisible()) {
            return;
        }

        mStatistics.Draw(DeltaTime, World, Render, mImageOrigin, mImageSize, mDrawList);
    }

    Editor::FViewportStatistics* Editor::FEditorViewport::GetStatistics() {
        return &mStatistics;
    }

    /**
     * @brief 창 표시와 도킹에 사용하는 이름을 반환합니다.
     */
    const Core::FString& Editor::FEditorViewport::GetName() const {
        return mName;
    }

    /**
     * @brief 소유한 뷰포트 클라이언트의 비소유 주소를 반환합니다.
     */
    Editor::FEditorViewportClient* Editor::FEditorViewport::GetClient() {
        return &mClient;
    }

    Engine::FViewportClient* Editor::FEditorViewport::GetActiveClient() {
        return mPlayClient != nullptr ? mPlayClient : &mClient;
    }

    void Editor::FEditorViewport::SetPlayClient(Engine::FViewportClient* Client) {
        mClient.GetGizmo().Cancel();
        mMouseCaptured = false;
        mPlayClient = Client;
        mPlayInputCaptured = false;

        if (mViewport != nullptr) {
            mViewport->SetClient(GetActiveClient());
        }
    }

    bool Editor::FEditorViewport::IsPlayInputCaptured() const {
        return mPlayInputCaptured;
    }

    /**
     * @brief 초기화된 장면 뷰포트의 비소유 주소를 반환합니다.
     */
    Engine::FSceneViewport* Editor::FEditorViewport::GetSceneViewport() const {
        return mViewport.get();
    }

    /**
     * @brief 장면 뷰포트가 소유한 컬러 타깃의 비소유 주소를 반환합니다.
     */
    Engine::FRenderTarget* Editor::FEditorViewport::GetRenderTarget() const {
        return mViewport != nullptr ? mViewport->GetRenderTarget() : nullptr;
    }

    /**
     * @brief 장면 타깃을 표시하고 뷰포트 입력 및 크기 변경을 전달합니다.
     * @note 이미지가 표시되지 않을 때에도 진행 중인 드래그의 종료 입력을 전달합니다.
     */
    void Editor::FEditorViewport::DrawUI(float DeltaTime) {
        if (ImGui::Begin(mName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs)) {
            DrawContents(DeltaTime);
        } else {
            ReleaseInput();
        }

        ImGui::End();
    }

    bool Editor::FEditorViewport::IsMouseCaptured() const {
        return mMouseCaptured;
    }

    void Editor::FEditorViewport::ReleaseInput() {
        mClient.GetGizmo().Cancel();
        mMouseCaptured = false;
        mPlayInputCaptured = false;
        mImageSize = {};
        mDrawList = nullptr;
    }

    void Editor::FEditorViewport::DrawContents(float DeltaTime, bool AllowInput) {
        mImageSize = {};
        mDrawList = nullptr;

        if (mViewport == nullptr) {
            return;
        }

        bool Submitted{};

        {
            if (mPlayClient != nullptr) {
                ImGui::TextDisabled(mPlayInputCaptured ? "Playing | Shift+F1: Release input | Esc: Stop" : "Playing | Click viewport to control | Esc: Stop");
            } else {
                ImGui::BeginDisabled(!AllowInput);
                Editor::ViewportToolbar::Draw(&mClient);
                ImGui::EndDisabled();
            }

            const ImVec2 Available{ImGui::GetContentRegionAvail()};

            if (Available.x >= 1.0f && Available.y >= 1.0f && mViewport->Resize(static_cast<Core::Uint32>(Available.x), static_cast<Core::Uint32>(Available.y))) {
                Engine::FRenderTarget* Target{mViewport->GetRenderTarget()};
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Target->GetNativeTexture()))}, Available);

                const ImVec2 Origin{ImGui::GetItemRectMin()};
                mImageOrigin = {Origin.x, Origin.y};
                mImageSize = {Available.x, Available.y};
                mDrawList = ImGui::GetWindowDrawList();

                const ImGuiIO& Input{ImGui::GetIO()};
                Engine::FViewportInput State{};
                const float ScaleX{Target->GetWidth() / Available.x};
                const float ScaleY{Target->GetHeight() / Available.y};

                State.mMouse = {(Input.MousePos.x - Origin.x) * ScaleX, (Input.MousePos.y - Origin.y) * ScaleY};
                State.mDelta = {Input.MouseDelta.x * ScaleX, Input.MouseDelta.y * ScaleY};

                State.mHovered = AllowInput && ImGui::IsItemHovered();

                if (State.mHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
                    ImGui::SetWindowFocus();

                    if (!mMouseCaptured) {
                        State.mDelta = {};
                    }

                    mMouseCaptured = true;
                }

                State.mFocused = AllowInput && ImGui::IsWindowFocused() && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);

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

                bool AssetDrag{};

                if (AllowInput && mPlayClient == nullptr && ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload(Editor::EditorAssetDragDrop::MeshPayload, ImGuiDragDropFlags_AcceptBeforeDelivery)}) {
                        AssetDrag = true;

                        if (Payload->IsDelivery() && Payload->Data != nullptr && Payload->DataSize > 1) {
                            const auto* Data{static_cast<const char*>(Payload->Data)};

                            if (Data[Payload->DataSize - 1] == '\0') {
                                const std::filesystem::path Path{Core::FilePath::FromUtf8(std::string_view{Data, static_cast<std::size_t>(Payload->DataSize - 1)})};
                                mClient.DropMesh(Path, State.mMouse, mViewport->GetSize());
                            }
                        }
                    }

                    if (const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload(Editor::EditorAssetDragDrop::ActorPayload, ImGuiDragDropFlags_AcceptBeforeDelivery)}) {
                        AssetDrag = true;

                        if (Payload->IsDelivery() && Payload->Data != nullptr && Payload->DataSize > 1) {
                            const auto* Data{static_cast<const char*>(Payload->Data)};

                            if (Data[Payload->DataSize - 1] == '\0') {
                                mClient.DropActor(Core::FString{Data, static_cast<std::size_t>(Payload->DataSize - 1)}, State.mMouse, mViewport->GetSize());
                            }
                        }
                    }

                    ImGui::EndDragDropTarget();
                }

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

                for (const Editor::FViewportCommandInfo& Info : Editor::FViewportCommandInfo::GetCommands()) {
                    if (mPlayClient == nullptr && Info.mInput != nullptr) {
                        const ImGuiKey Key{ShortcutKey(Info.mShortcut)};
                        State.*Info.mInput = Keyboard && Key != ImGuiKey_None && ImGui::IsKeyPressed(Key, false);
                    }
                }

                if (!AssetDrag && AllowInput) {
                    if (!mMouseCaptured && mPlayClient == nullptr) {
                        State.mLeftDown = false;
                        State.mRightDown = false;
                        State.mMiddleDown = false;
                        State.mDelta = {};
                    }

                    mViewport->Input(State, DeltaTime);
                }

                Submitted = true;
            }
        }

        if (!Submitted && mClient.GetGizmo().IsDragging() && ((mClient.GetGizmo().IsPivotDrag() ? !ImGui::IsMouseDown(ImGuiMouseButton_Middle) : !ImGui::IsMouseDown(ImGuiMouseButton_Left)) || ImGui::IsKeyPressed(ImGuiKey_Escape))) {
            Engine::FViewportInput State{};
            State.mLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            State.mMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
            State.mEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);
            mViewport->Input(State, DeltaTime);
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            mMouseCaptured = false;
        }
    }

}
