/**
 * @file FScriptInput.cpp
 * @brief 공통 입력의 게임 경로를 C# 호출에 제공합니다.
 * @ingroup Script
 */

#include "Script/FScriptInput.h"

namespace Script {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 스크립트의 게임 입력 조회에 사용할 공통 입력의 비소유 주소입니다.
         */
        const Core::FInput* sInput{};
    }

    /**
     * @brief 읽을 입력 상태를 연결합니다.
     */
    void Script::FScriptInput::SetSource(const Core::FInput* Input) {
        sInput = Input;
    }

    /**
     * @brief 키가 눌려 있는지 검사합니다.
     */
    bool Script::FScriptInput::IsDown(Core::EKey Key) {
        return sInput != nullptr && sInput->IsDown(Key, Core::EInputRoute::Game);
    }

    /**
     * @brief 이번 프레임에 키를 눌렀는지 검사합니다.
     */
    bool Script::FScriptInput::WasPressed(Core::EKey Key) {
        return sInput != nullptr && sInput->WasPressed(Key, Core::EInputRoute::Game);
    }

    /**
     * @brief 이번 프레임에 키를 뗐는지 검사합니다.
     */
    bool Script::FScriptInput::WasReleased(Core::EKey Key) {
        return sInput != nullptr && sInput->WasReleased(Key, Core::EInputRoute::Game);
    }

    /**
     * @brief 마우스 위치를 반환합니다.
     */
    Core::FVector2 Script::FScriptInput::GetMousePosition() {
        return sInput != nullptr ? sInput->GetMousePosition(Core::EInputRoute::Game) : Core::FVector2{};
    }

    /**
     * @brief 이번 프레임의 마우스 이동량을 반환합니다.
     */
    Core::FVector2 Script::FScriptInput::GetMouseDelta() {
        return sInput != nullptr ? sInput->GetMouseDelta(Core::EInputRoute::Game) : Core::FVector2{};
    }

    /**
     * @brief 이번 프레임의 마우스 휠 이동량을 반환합니다.
     */
    float Script::FScriptInput::GetMouseWheel() {
        return sInput != nullptr ? sInput->GetMouseWheel(Core::EInputRoute::Game) : 0.0f;
    }

}
