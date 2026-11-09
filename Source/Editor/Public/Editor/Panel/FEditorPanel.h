/**
 * @file FEditorPanel.h
 * @brief ImGui 패널의 이름, 가시성과 내용 그리기 수명을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Editor {

    class FEditorContext;

    /**
     * @brief ImGui 패널의 이름, 가시성과 내용 그리기 수명을 관리합니다.
     * @ingroup Editor
     */
    class FEditorPanel {
    public:
        /**
         * @brief FEditorPanel의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in,out] Context 패널이 조회하고 편집할 에디터 컨텍스트입니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Closable 사용자가 패널을 닫을 수 있는지 지정합니다.
         */
        FEditorPanel(Editor::FEditorContext* Context, Core::FString Name, bool Closable = false);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~FEditorPanel();

        /**
         * @brief 레지스트리에 등록된 고유 객체이므로 복사·이동 생성을 금지합니다.
         */
        FEditorPanel(const Editor::FEditorPanel&) = delete;

        /**
         * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Editor::FEditorPanel& operator=(const Editor::FEditorPanel&) = delete;

    public:
        /**
         * @brief 패널이 표시 상태이면 ImGui 창을 열고 내용을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         */
        void Draw(float DeltaTime);

        /**
         * @brief 패널 창 이름을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 패널 창의 표시 여부를 반환합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsVisible() const;

        /**
         * @brief 패널 창의 표시 상태를 설정합니다.
         * @param[in] Visible 창이나 패널을 표시할지 지정합니다.
         */
        void SetVisible(bool Visible);

        /**
         * @brief 패널을 표시하고 다음 그리기에서 창에 포커스를 줍니다.
         * @note 도킹된 패널은 해당 탭이 앞으로 나옵니다.
         */
        void Open();

        /**
         * @brief 연결된 공통 편집 컨텍스트를 반환합니다.
         * @return 연결된 공통 편집 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* GetEditorContext() const;

        /**
         * @brief 패널이 사용할 편집 컨텍스트의 비소유 주소를 교체합니다.
         * @param[in] Context 패널이 조회하고 편집할 에디터 컨텍스트입니다.
         */
        void SetEditorContext(Editor::FEditorContext* Context);

    private:
        /**
         * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         */
        virtual void DrawContent(float DeltaTime) = 0;

    private:
        /**
         * @brief 연결된 공통 편집 컨텍스트입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief 패널 창 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 패널 닫기 버튼을 표시할지 여부입니다.
         */
        bool mClosable{};

        /**
         * @brief 패널 표시 여부입니다.
         */
        bool mVisible{true};

        /**
         * @brief 다음 그리기에서 창에 포커스를 줄지 여부입니다.
         */
        bool mFocusRequested{};
    };

}
