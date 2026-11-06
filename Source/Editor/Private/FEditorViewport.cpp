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
     * @brief 뷰포트 단축키 정의를 공용 입력 키로 연결합니다.
     */
    namespace {
        /**
         * @brief 공통 명령에 정의된 단축키 문자를 입력 키 식별자로 변환합니다.
         * @param[in] Shortcut 영문 대문자, 숫자, 공백 또는 탭으로 표현한 단축키입니다.
         * @return 대응하는 입력 키이며 지원하지 않는 문자나 0이면 Core::EKey::None입니다.
         */
        Core::EKey ShortcutKey(char Shortcut) {
            if ((Shortcut >= 'A' && Shortcut <= 'Z') || (Shortcut >= '0' && Shortcut <= '9')) {
                return static_cast<Core::EKey>(Shortcut);
            }

            return Shortcut == ' ' ? Core::EKey::Space : Shortcut == '\t' ? Core::EKey::Tab : Core::EKey::None;
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

    /**
     * @brief 뷰포트 이미지 영역 위에 월드와 렌더링 통계를 표시합니다.
     */
    void Editor::FEditorViewport::DrawStatistics(float DeltaTime, const Engine::UWorld* World, const RenderCore::FRenderStats& Render) {
        if (mImageSize.x < 1.0f || mImageSize.y < 1.0f || mDrawList == nullptr || !mStatistics.IsVisible()) {
            return;
        }

        mStatistics.Draw(DeltaTime, World, Render, mImageOrigin, mImageSize, mDrawList);
    }

    /**
     * @brief 이 뷰포트가 소유한 통계 표시 객체의 주소를 반환합니다.
     */
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

    /**
     * @brief PIE 연결 상태에 따라 현재 사용할 뷰포트 클라이언트를 반환합니다.
     */
    Engine::FViewportClient* Editor::FEditorViewport::GetActiveClient() {
        return mPlayClient != nullptr ? mPlayClient : &mClient;
    }

    /**
     * @brief PIE에 사용할 클라이언트를 연결하거나 해제하고 입력 캡처를 초기화합니다.
     */
    void Editor::FEditorViewport::SetPlayClient(Engine::FViewportClient* Client) {
        mClient.GetGizmo().Cancel();
        mMouseCaptured = false;
        mPlayClient = Client;
        mPlayInputCaptured = false;

        if (mViewport != nullptr) {
            mViewport->SetClient(GetActiveClient());
        }
    }

    /**
     * @brief PIE 뷰포트가 마우스와 키보드 입력을 캡처했는지 반환합니다.
     */
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
    void Editor::FEditorViewport::DrawUI(const Core::FInput& Input, float DeltaTime) {
        if (ImGui::Begin(mName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs)) {
            DrawContents(Input, DeltaTime);
        } else {
            ReleaseInput();
        }

        ImGui::End();
    }

    /**
     * @brief 뷰포트가 마우스 입력을 캡처 중인지 검사합니다.
     */
    bool Editor::FEditorViewport::IsMouseCaptured() const {
        return mMouseCaptured;
    }

    /**
     * @brief 기즈모 조작과 마우스·PIE 입력 캡처를 해제합니다.
     */
    void Editor::FEditorViewport::ReleaseInput() {
        mClient.GetGizmo().Cancel();
        mMouseCaptured = false;
        mPlayInputCaptured = false;
        mImageSize = {};
        mDrawList = nullptr;
    }

    /**
     * @brief 뷰포트 타깃 이미지·툴바를 표시하고 입력과 리사이즈를 전달합니다.
     */
    void Editor::FEditorViewport::DrawContents(const Core::FInput& Input, float DeltaTime, bool AllowInput) {
        using Core::EKey;

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

                const bool Hovered{AllowInput && ImGui::IsItemHovered()};
                const bool LeftPressed{Input.WasPressed(EKey::MouseLeft)};
                const bool Blocked{!AllowInput || !Input.IsFocused() || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)};

                if (mPlayClient != nullptr) {
                    if (Blocked || (Input.IsDown(EKey::Shift) && Input.WasPressed(EKey::F1))) {
                        mPlayInputCaptured = false;
                    } else if (Hovered && LeftPressed) {
                        ImGui::SetWindowFocus();

                        mPlayInputCaptured = true;
                    } else if (Input.LostFocusThisFrame() || ImGui::GetIO().WantTextInput || !ImGui::IsWindowFocused()) {
                        mPlayInputCaptured = false;
                    }

                    return;
                }

                const Core::FVector2 Mouse{Input.GetMousePosition()};
                const Core::FVector2 Delta{Input.GetMouseDelta()};
                Engine::FViewportInput State{};
                const float ScaleX{Target->GetWidth() / Available.x};
                const float ScaleY{Target->GetHeight() / Available.y};

                State.mMouse = {(Mouse.x - Origin.x) * ScaleX, (Mouse.y - Origin.y) * ScaleY};
                State.mDelta = {Delta.x * ScaleX, Delta.y * ScaleY};

                State.mHovered = Hovered;

                if (Hovered && (LeftPressed || Input.WasPressed(EKey::MouseRight) || Input.WasPressed(EKey::MouseMiddle))) {
                    ImGui::SetWindowFocus();

                    if (!mMouseCaptured) {
                        State.mDelta = {};
                    }

                    mMouseCaptured = true;
                }

                State.mFocused = !Blocked && ImGui::IsWindowFocused();

                State.mLeftPressed = LeftPressed;
                State.mLeftDown = Input.IsDown(EKey::MouseLeft);
                State.mLeftReleased = Input.WasReleased(EKey::MouseLeft);
                State.mRightDown = Input.IsDown(EKey::MouseRight);
                State.mMiddleDown = Input.IsDown(EKey::MouseMiddle);
                State.mMiddlePressed = Input.WasPressed(EKey::MouseMiddle);
                State.mMiddleReleased = Input.WasReleased(EKey::MouseMiddle);
                State.mControl = Input.IsDown(EKey::Control);

                State.mAlt = Input.IsDown(EKey::Alt);
                State.mWheel = Input.GetMouseWheel();
                State.mEscape = Input.WasPressed(EKey::Escape);

                bool AssetDrag{};

                if (AllowInput && ImGui::BeginDragDropTarget()) {
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

                const bool Keyboard{State.mFocused && !ImGui::GetIO().WantTextInput && !Input.IsDown(EKey::Control) && !Input.IsDown(EKey::Alt)};

                State.mDelete = Keyboard && Input.WasPressed(EKey::Delete);
                State.mFocusSelection = Keyboard && Input.WasPressed(EKey::F);

                State.mForward = Keyboard && Input.IsDown(EKey::W);
                State.mBackward = Keyboard && Input.IsDown(EKey::S);
                State.mLeft = Keyboard && Input.IsDown(EKey::A);
                State.mRight = Keyboard && Input.IsDown(EKey::D);
                State.mUp = Keyboard && Input.IsDown(EKey::E);
                State.mDown = Keyboard && Input.IsDown(EKey::Q);
                State.mFast = Input.IsDown(EKey::Shift);

                for (const Editor::FViewportCommandInfo& Info : Editor::FViewportCommandInfo::GetCommands()) {
                    if (Info.mInput != nullptr) {
                        const EKey Key{ShortcutKey(Info.mShortcut)};

                        State.*Info.mInput = Keyboard && Key != Core::EKey::None && Input.WasPressed(Key);
                    }
                }

                if (!AssetDrag && AllowInput) {
                    if (!mMouseCaptured) {
                        State.mLeftDown = false;
                        State.mRightDown = false;
                        State.mMiddleDown = false;
                        State.mDelta = {};
                    }

                    mClient.Input(mViewport.get(), State, DeltaTime);
                }

                Submitted = true;
            }
        }

        if (!Submitted) {
            mPlayInputCaptured = false;
        }

        if (!Submitted && mClient.GetGizmo().IsDragging() && ((mClient.GetGizmo().IsPivotDrag() ? !Input.IsDown(EKey::MouseMiddle) : !Input.IsDown(EKey::MouseLeft)) || Input.WasPressed(EKey::Escape))) {
            Engine::FViewportInput State{};
            State.mLeftDown = Input.IsDown(EKey::MouseLeft);
            State.mMiddleDown = Input.IsDown(EKey::MouseMiddle);
            State.mEscape = Input.WasPressed(EKey::Escape);

            mClient.Input(mViewport.get(), State, DeltaTime);
        }

        if (!Input.IsDown(EKey::MouseLeft) && !Input.IsDown(EKey::MouseRight) && !Input.IsDown(EKey::MouseMiddle)) {
            mMouseCaptured = false;
        }
    }

}
