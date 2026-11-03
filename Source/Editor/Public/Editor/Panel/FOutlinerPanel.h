/**
 * @file FOutlinerPanel.h
 * @brief 월드의 액터 목록과 선택 상태를 표시합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"

#include <array>

namespace Editor {

    /**
     * @brief 월드의 액터 목록과 선택 상태를 표시합니다.
     * @ingroup Editor
     */
    class FOutlinerPanel final : public Editor::FEditorPanel {
    public:
        /**
         * @brief FOutlinerPanel의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in,out] Context 패널이 조회하고 편집할 에디터 컨텍스트입니다.
         */
        explicit FOutlinerPanel(Editor::FEditorContext* Context);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FOutlinerPanel() override;

    private:
        /**
         * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         */
        void DrawContent(float DeltaTime) override;

    private:
        /**
         * @brief 액터의 이름과 타입을 거르는 검색어 버퍼입니다.
         */
        std::array<char, 128> mSearch{};
    };

}
