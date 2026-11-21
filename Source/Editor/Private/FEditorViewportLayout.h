/**
 * @file FEditorViewportLayout.h
 * @brief 뷰포트 분할 프리셋·최대화·활성 입력과 ImGui 설정 저장을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Input.h"
#include "Core/Math/FVector4.h"
#include "Editor/EEditorViewportLayoutPreset.h"
#include "Core/Types.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "RenderCore/IRendererModule.h"

#include <memory>
#include <imgui.h>
#include <imgui_internal.h>

namespace Editor {

    /**
     * @brief 뷰포트 분할 프리셋·최대화·활성 입력과 ImGui 설정 저장을 관리합니다.
     */
    class FEditorViewportLayout {
    private:
        /**
         * @brief 분할 방향·비율 또는 뷰포트 슬롯을 나타내는 배치 트리 노드입니다.
         */
        struct FLayoutNode {
            /**
             * @brief 분할 노드의 첫 번째 자식 인덱스이며 없으면 -1입니다.
             */
            Int32 mFirst{-1};

            /**
             * @brief 분할 노드의 두 번째 자식 인덱스이며 없으면 -1입니다.
             */
            Int32 mSecond{-1};

            /**
             * @brief 리프 노드가 표시할 뷰포트 슬롯 인덱스입니다.
             */
            Index_t mSlot{};

            /**
             * @brief 분할 비율 배열에서 사용할 인덱스입니다.
             */
            Index_t mRatio{};

            /**
             * @brief 노드를 좌우 열로 분할할지 나타냅니다.
             */
            bool mColumns{};
        };

        /**
         * @brief 뷰포트 배치 프리셋의 이름·슬롯 수·분할 트리와 초기 비율입니다.
         */
        struct FPresetDefinition {
            /**
             * @brief 뷰포트 분할 프리셋의 표시 이름입니다.
             */
            const char* mName{};

            /**
             * @brief 프리셋이 표시하는 뷰포트 슬롯 수입니다.
             */
            Index_t mCount{};

            /**
             * @brief 프리셋의 분할·뷰포트 배치 트리입니다.
             */
            Core::TStaticArray<FLayoutNode, 7> mNodes{};

            /**
             * @brief 프리셋으로 전환할 때 복원할 초기 분할 비율입니다.
             */
            Core::TStaticArray<float, 3> mRatios{0.5f, 0.5f, 0.5f};
        };

    public:
        /**
         * @brief FEditorViewportLayout의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Viewport 레벨 편집기 주 뷰포트의 비소유 주소입니다.
         * @param[in] PlaySession PIE 실행과 입력 상태를 제공하는 세션입니다.
         */
        FEditorViewportLayout(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorViewportLayout();

        /**
         * @brief 중복된 수명·연결 상태를 방지하기 위해 복사 생성을 금지합니다.
         */
        FEditorViewportLayout(const Editor::FEditorViewportLayout&) = delete;

        /**
         * @brief 중복된 수명·연결 상태를 방지하기 위해 대입을 금지합니다.
         */
        Editor::FEditorViewportLayout& operator=(const Editor::FEditorViewportLayout&) = delete;

    public:
        /**
         * @brief 추가 직교 뷰포트와 배치 설정 핸들러를 초기화합니다.
         * @param[in] Renderer 렌더 대상과 GPU 리소스를 제공하는 렌더러입니다.
         * @return 모든 뷰포트 리소스를 준비했으면 true입니다.
         */
        bool Initialize(RenderCore::IRendererModule* Renderer);

        /**
         * @brief 배치 설정 핸들러와 추가 직교 뷰포트 리소스를 해제합니다.
         */
        void Shutdown();

        /**
         * @brief 이번 프레임에 표시할 뷰포트 슬롯 상태를 초기화합니다.
         */
        void BeginFrame();

        /**
         * @brief 레이아웃의 표시 중인 뷰포트와 입력을 처리합니다.
         * @param[in] Input 이번 프레임의 키·마우스 입력 상태입니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void DrawContents(const Core::FInput& Input, float DeltaTime);

        /**
         * @brief 표시 중인 뷰포트의 월드 렌더링을 제출합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Draw(float DeltaTime);

        /**
         * @brief 뷰포트 분할 프리셋을 선택하고 해당 배치를 적용합니다.
         * @param[in] Preset 사용할 뷰포트 분할 프리셋입니다.
         */
        void SetPreset(Editor::EEditorViewportLayoutPreset Preset);

        /**
         * @brief 현재 뷰포트 분할 프리셋을 반환합니다.
         * @return 선택된 프리셋입니다.
         */
        Editor::EEditorViewportLayoutPreset GetPreset() const;

        /**
         * @brief 분할 프리셋의 표시 이름을 조회합니다.
         * @param[in] Preset 사용할 뷰포트 분할 프리셋입니다.
         * @return 프리셋의 표시 문자열입니다.
         */
        static const char* GetPresetName(Editor::EEditorViewportLayoutPreset Preset);

        /**
         * @brief 지정한 뷰포트의 단독 표시와 분할 배치를 전환합니다.
         * @param[in] Index 조회하거나 변경할 0부터 시작하는 인덱스입니다.
         */
        void ToggleMaximize(Index_t Index);

        /**
         * @brief 최대화를 해제하고 선택한 프리셋의 분할 배치를 복원합니다.
         */
        void RestoreViewports();

        /**
         * @brief 창 또는 뷰포트가 최대화 상태인지 검사합니다.
         * @return 최대화 상태이면 true입니다.
         */
        bool IsMaximized() const;

        /**
         * @brief 현재 실행과 입력 상태에서 뷰포트 배치를 바꿀 수 있는지 검사합니다.
         * @return 배치를 변경할 수 있으면 true입니다.
         */
        bool CanChangeLayout() const;

        /**
         * @brief 현재 활성 뷰포트를 반환합니다.
         * @return 활성 뷰포트의 비소유 주소입니다.
         */
        Editor::FEditorViewport* GetActiveViewport() const;

        /**
         * @brief 슬롯 인덱스에 대응하는 뷰포트를 조회합니다.
         * @param[in] Index 조회하거나 변경할 0부터 시작하는 인덱스입니다.
         * @return 해당 뷰포트의 비소유 주소이며 유효한 슬롯이 아니면 nullptr입니다.
         */
        Editor::FEditorViewport* GetViewport(Index_t Index) const;

        /**
         * @brief 현재 가로 분할 비율을 반환합니다.
         * @return 가로 분할에 사용하는 비율입니다.
         */
        float GetHorizontalRatio() const;

        /**
         * @brief 현재 세로 분할 비율을 반환합니다.
         * @return 세로 분할에 사용하는 비율입니다.
         */
        float GetVerticalRatio() const;

        /**
         * @brief 유효한 가로·세로 분할 비율을 적용합니다.
         * @param[in] Horizontal 가로 방향 분할 비율입니다.
         * @param[in] Vertical 세로 방향 분할 비율입니다.
         * @return 비율을 적용할 수 있으면 true입니다.
         */
        bool SetSplitRatios(float Horizontal, float Vertical);

    private:
        /**
         * @brief 분할 프리셋의 정적 배치 정의를 조회합니다.
         * @param[in] Preset 사용할 뷰포트 분할 프리셋입니다.
         * @return 프리셋의 노드·이름·초기 비율 정의입니다.
         */
        static const Editor::FEditorViewportLayout::FPresetDefinition& GetDefinition(Editor::EEditorViewportLayoutPreset Preset);

        /**
         * @brief 뷰포트별 활성 상태와 편집 컨텍스트 연결을 갱신합니다.
         */
        void UpdateViewportContexts();

        /**
         * @brief 한 뷰포트를 그리고 입력을 캡처한 뷰포트를 기록합니다.
         * @param[in] Index 조회하거나 변경할 0부터 시작하는 인덱스입니다.
         * @param[in] Input 이번 프레임의 키·마우스 입력 상태입니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         * @param[in,out] CapturedViewport 입력 캡처를 확보한 뷰포트를 전달받을 비소유 포인터입니다.
         */
        void DrawViewport(Index_t Index, const Core::FInput& Input, float DeltaTime, Editor::FEditorViewport*& CapturedViewport);

        /**
         * @brief 분할 트리 노드를 재귀적으로 배치하고 뷰포트와 구분선을 그립니다.
         * @param[in] Definition 사용할 프리셋의 배치 트리 정의입니다.
         * @param[in] NodeIndex 그릴 배치 트리 노드의 인덱스입니다.
         * @param[in] Bounds 배치 노드의 화면 위치와 가로·세로 크기입니다.
         * @param[in] Input 이번 프레임의 키·마우스 입력 상태입니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         * @param[in,out] CapturedViewport 입력 캡처를 확보한 뷰포트를 전달받을 비소유 포인터입니다.
         */
        void DrawLayoutNode(const Editor::FEditorViewportLayout::FPresetDefinition& Definition, Index_t NodeIndex, const FVector4& Bounds, const Core::FInput& Input, float DeltaTime, Editor::FEditorViewport*& CapturedViewport);

        /**
         * @brief 해당 뷰포트의 배치 프리셋 메뉴와 최대화 전환 버튼을 그립니다.
         * @param[in] Index 조회하거나 변경할 0부터 시작하는 인덱스입니다.
         */
        void DrawLayoutControls(Index_t Index);

        /**
         * @brief 분할 미리 보기를 포함한 프리셋 선택 버튼을 그립니다.
         * @param[in] Preset 사용할 뷰포트 분할 프리셋입니다.
         */
        void DrawPresetButton(Editor::EEditorViewportLayoutPreset Preset);

        /**
         * @brief 배치 트리의 축소 미리 보기를 재귀적으로 그립니다.
         * @param[in] Definition 사용할 프리셋의 배치 트리 정의입니다.
         * @param[in] NodeIndex 그릴 배치 트리 노드의 인덱스입니다.
         * @param[in] Bounds 배치 노드의 화면 위치와 가로·세로 크기입니다.
         * @param[in] Color 그리기에 사용할 패킹된 색상입니다.
         */
        void DrawPreviewNode(const Editor::FEditorViewportLayout::FPresetDefinition& Definition, Index_t NodeIndex, const FVector4& Bounds, Uint32 Color) const;

        /**
         * @brief ImGui 설정 섹션을 뷰포트 배치의 읽기 대상에 연결합니다.
         * @param[in] Context 설정 콜백을 실행하는 ImGui 컨텍스트입니다.
         * @param[in] Handler 해당 이벤트를 처리할 콜백입니다.
         * @param[in] Name 읽기 대상 ImGui 설정 섹션 이름입니다.
         * @return 인식한 섹션의 설정 대상이며 해당하지 않으면 nullptr입니다.
         */
        static void* ReadSettingsOpen(ImGuiContext* Context, ImGuiSettingsHandler* Handler, const char* Name);

        /**
         * @brief ImGui 설정 한 줄에서 프리셋·분할 비율·뷰포트 표시 상태를 읽습니다.
         * @param[in] Context 설정 콜백을 실행하는 ImGui 컨텍스트입니다.
         * @param[in] Handler 해당 이벤트를 처리할 콜백입니다.
         * @param[in] Entry 설정 섹션을 읽을 대상 객체입니다.
         * @param[in] Line 해석할 ImGui 설정 한 줄입니다.
         */
        static void ReadSettingsLine(ImGuiContext* Context, ImGuiSettingsHandler* Handler, void* Entry, const char* Line);

        /**
         * @brief 뷰포트 배치 상태를 ImGui 설정 텍스트 버퍼에 기록합니다.
         * @param[in] Context 설정 콜백을 실행하는 ImGui 컨텍스트입니다.
         * @param[in] Handler 해당 이벤트를 처리할 콜백입니다.
         * @param[out] Buffer 설정 텍스트를 추가할 ImGui 버퍼입니다.
         */
        static void WriteSettings(ImGuiContext* Context, ImGuiSettingsHandler* Handler, ImGuiTextBuffer* Buffer);

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief 주 뷰포트의 비소유 주소 또는 별도 PIE 창의 소유 뷰포트입니다.
         */
        Editor::FEditorViewport* mViewport{};

        /**
         * @brief PIE의 월드·재생·입력 상태를 관리하는 공유 세션입니다.
         */
        Editor::FPlayInEditorSession* mPlaySession{};

        /**
         * @brief 주 원근 뷰포트 외의 직교 뷰포트 소유 목록입니다.
         */
        Core::TStaticArray<std::unique_ptr<Editor::FEditorViewport>, 3> mOrthographicViewports{};

        /**
         * @brief 탭·위젯 또는 뷰포트 슬롯의 표시 여부입니다.
         */
        Core::TStaticArray<bool, 4> mVisible{};

        /**
         * @brief 현재 입력 대상으로 선택된 뷰포트 슬롯입니다.
         */
        Index_t mActiveViewport{};

        /**
         * @brief 최대화 상태에서 단독 표시할 뷰포트 슬롯입니다.
         */
        Index_t mSingleViewport{};

        /**
         * @brief 현재 선택한 뷰포트 분할 프리셋입니다.
         */
        Editor::EEditorViewportLayoutPreset mPreset{Editor::EEditorViewportLayoutPreset::FourGrid};

        /**
         * @brief 현재 분할 트리에서 사용하는 노드별 분할 비율입니다.
         */
        Core::TStaticArray<float, 3> mRatios{0.5f, 0.5f, 0.5f};

        /**
         * @brief 하나의 뷰포트만 확대 표시하는지 나타냅니다.
         */
        bool mMaximized{true};
    };
}
