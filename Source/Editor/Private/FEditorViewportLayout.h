/**
 * @file FEditorViewportLayout.h
 * @brief 활성 뷰와 입력 캡처를 유지하며 단일·4분할 뷰를 배치합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <array>
#include <memory>

struct ImGuiContext;
struct ImGuiSettingsHandler;
struct ImGuiTextBuffer;

namespace RenderCore {
    class IRendererModule;
}

namespace Editor {

    class FEditorContext;
    class FEditorViewport;
    class FPlayInEditorSession;

    /**
     * @brief 활성 뷰와 입력 캡처를 유지하며 단일·4분할 뷰를 배치합니다.
     */
    class FEditorViewportLayout {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FEditorViewportLayout);

    public:
        /**
         * @brief 전달된 설정과 기본값으로 FEditorViewportLayout 객체를 생성합니다.
         * @param[in] Context 그리기 또는 편집에 사용할 컨텍스트의 비소유 주소입니다.
         * @param[in] Viewport 기본 원근 뷰포트의 비소유 주소입니다.
         * @param[in] PlaySession PIE 생명주기를 제공하는 세션입니다.
         */
        FEditorViewportLayout(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession);

        /**
         * @brief 객체를 소멸하고 소유한 GPU 자원 또는 UV 제어 상태를 해제합니다.
         */
        ~FEditorViewportLayout();

        /**
         * @brief 소유 자원이 중복되는 복사 생성을 금지합니다.
         */
        FEditorViewportLayout(const Editor::FEditorViewportLayout&) = delete;

        /**
         * @brief 소유 자원과 참조가 중복되는 복사 대입을 금지합니다.
         */
        Editor::FEditorViewportLayout& operator=(const Editor::FEditorViewportLayout&) = delete;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
         */
        ReflectStruct(FEditorViewportLayout);

        /**
         * @brief 뷰포트 또는 스프라이트의 GPU 자원과 기본 상태를 초기화합니다.
         * @param[in] Renderer 뷰포트 자원을 생성할 렌더 모듈입니다.
         * @return 검증 또는 요청한 처리에 성공하면 true입니다.
         */
        bool Initialize(RenderCore::IRendererModule* Renderer);

        /**
         * @brief 입력 캡처를 해제하고 소유 뷰포트 자원을 정리합니다.
         */
        void Shutdown();

        /**
         * @brief 단일 또는 4분할 뷰포트 UI를 표시합니다.
         * @param[in] DeltaTime 이전 갱신 이후의 초 단위 시간입니다.
         */
        void DrawUI(float DeltaTime);

        /**
         * @brief 표시 중인 뷰포트의 렌더 요청을 제출합니다.
         * @param[in] DeltaTime 이전 갱신 이후의 초 단위 시간입니다.
         */
        void Draw(float DeltaTime);

        /**
         * @brief 활성 뷰를 보존하며 단일·4분할 레이아웃을 전환합니다.
         * @param[in] QuadView 4분할 레이아웃을 사용할지 지정합니다.
         */
        void SetQuadView(bool QuadView);

        /**
         * @brief 4분할 레이아웃 여부인지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsQuadView() const;

        /**
         * @brief 입력 캡처나 PIE 상태가 레이아웃 변경을 허용하는지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool CanChangeLayout() const;

        /**
         * @brief 가장 최근 입력을 받은 유효한 뷰포트를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
         */
        Editor::FEditorViewport* GetActiveViewport() const;

        /**
         * @brief 0 기반 슬롯에 해당하는 뷰포트를 조회합니다.
         * @param[in] Index 0 기반 뷰포트 슬롯입니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
         */
        Editor::FEditorViewport* GetViewport(Core::Uint32 Index) const;

        /**
         * @brief 분할 비율을 반환합니다.
         * @return 가로 분할 위치의 비율입니다.
         */
        float GetHorizontalRatio() const;

        /**
         * @brief 분할 비율을 반환합니다.
         * @return 세로 분할 위치의 비율입니다.
         */
        float GetVerticalRatio() const;

        /**
         * @brief 유효한 분할 비율을 적용합니다.
         * @param Horizontal 가로 비율입니다.
         * @param Vertical 세로 비율입니다.
         * @return 두 비율이 유효하면 true입니다.
         */
        bool SetSplitRatios(float Horizontal, float Vertical);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 현재 편집 또는 PIE 컨텍스트를 각 뷰포트에 연결합니다.
         */
        void UpdateViewportContexts();

        /**
         * @brief 한 뷰포트의 UI를 표시하고 입력을 캡처한 뷰를 갱신합니다.
         * @param[in] Index 0 기반 뷰포트 슬롯입니다.
         * @param[in] DeltaTime 이전 갱신 이후의 초 단위 시간입니다.
         * @param[in,out] CapturedViewport 입력을 캡처한 뷰포트의 비소유 주소입니다.
         */
        void DrawViewport(Core::Uint32 Index, float DeltaTime, Editor::FEditorViewport*& CapturedViewport);

        /**
         * @brief INI 레이아웃 섹션을 엽니다.
         * @param Context ImGui 컨텍스트입니다.
         * @param Handler 등록한 설정 핸들러입니다.
         * @param Name 섹션 이름입니다.
         * @return 일치하는 레이아웃 주소 또는 nullptr입니다.
         */
        static void* ReadSettingsOpen(ImGuiContext* Context, ImGuiSettingsHandler* Handler, const char* Name);

        /**
         * @brief 검증한 INI 레이아웃 값을 읽습니다.
         * @param Context ImGui 컨텍스트입니다.
         * @param Handler 등록한 설정 핸들러입니다.
         * @param Entry 레이아웃 주소입니다.
         * @param Line 읽을 설정 한 줄입니다.
         */
        static void ReadSettingsLine(ImGuiContext* Context, ImGuiSettingsHandler* Handler, void* Entry, const char* Line);

        /**
         * @brief 분할 비율과 뷰 모드를 INI에 씁니다.
         * @param Context ImGui 컨텍스트입니다.
         * @param Handler 등록한 설정 핸들러입니다.
         * @param Buffer 출력할 INI 버퍼입니다.
         */
        static void WriteSettings(ImGuiContext* Context, ImGuiSettingsHandler* Handler, ImGuiTextBuffer* Buffer);

    private:
        /**
         * @brief 편집 월드와 선택 상태를 제공하는 컨텍스트입니다.
         */
        Editor::FEditorContext* mContext{};
        ReflectProperty(mContext, "Context", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 기본 원근 뷰포트의 비소유 주소입니다.
         */
        Editor::FEditorViewport* mViewport{};
        ReflectProperty(mViewport, "Viewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief PIE 생명주기를 제공하는 세션입니다.
         */
        Editor::FPlayInEditorSession* mPlaySession{};
        ReflectProperty(mPlaySession, "PlaySession", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 4분할에 쓰는 정사영 뷰포트 소유 배열입니다.
         */
        std::array<std::unique_ptr<Editor::FEditorViewport>, 3> mOrthographicViewports{};
        ReflectProperty(mOrthographicViewports, "OrthographicViewports", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 프레임에 표시한 뷰포트 슬롯 배열입니다.
         */
        std::array<bool, 4> mVisible{};
        ReflectProperty(mVisible, "Visible", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 마지막 입력을 받은 활성 뷰포트입니다.
         */
        Core::Uint32 mActiveViewport{};
        ReflectProperty(mActiveViewport, "ActiveViewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 단일 레이아웃에 표시할 뷰포트입니다.
         */
        Core::Uint32 mSingleViewport{};
        ReflectProperty(mSingleViewport, "SingleViewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 4분할 레이아웃 여부입니다.
         */
        bool mQuadView{};
        ReflectProperty(mQuadView, "QuadView", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 4분할 가로 분할 비율입니다.
         */
        float mHorizontalRatio{0.5f};
        ReflectProperty(mHorizontalRatio, "HorizontalRatio", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 4분할 세로 분할 비율입니다.
         */
        float mVerticalRatio{0.5f};
        ReflectProperty(mVerticalRatio, "VerticalRatio", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
