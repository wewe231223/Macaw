/**
 * @file FConsolePanel.h
 * @brief 콘솔 로그와 명령 입력을 표시하는 에디터 패널입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"

#include <array>

namespace Editor {

    /**
     * @brief 콘솔 로그와 명령 입력을 표시하는 에디터 패널입니다.
     * @ingroup Editor
     */
    class FConsolePanel final : public Editor::FEditorPanel {
    public:
        /**
         * @brief FConsolePanel의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in,out] Context 패널이 조회하고 편집할 에디터 컨텍스트입니다.
         */
        explicit FConsolePanel(Editor::FEditorContext* Context);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FConsolePanel() override;

    public:
        /**
         * @brief 콘솔 패널에 메시지를 추가하고 스크롤 갱신을 요청합니다.
         * @param[in] Message 보고할 오류 내용입니다.
         */
        void AddLog(const Core::FString& Message);

        /**
         * @brief 수집한 콘솔 로그를 모두 지웁니다.
         */
        void Clear();

    private:
        /**
         * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         */
        void DrawContent(float DeltaTime) override;

    private:
        /**
         * @brief 마지막으로 표시한 로그 모델의 변경 세대입니다.
         */
        Core::Uint64 mLastLogRevision{};

        /**
         * @brief 널 종료된 콘솔 명령 입력 버퍼입니다.
         */
        std::array<char, 512> mConsoleInput{};

        /**
         * @brief 다음 그리기에서 로그 끝으로 이동할지 여부입니다.
         */
        bool mScrollToBottom{};
    };

}
