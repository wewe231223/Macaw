/**
 * @file FScriptInput.h
 * @brief C#의 Macaw.Input이 호출하는 게임 입력 조회 함수입니다.
 * @ingroup Script
 */

#pragma once

#include "Core/Input.h"
#include "Core/Math/FVector2.h"
#include "Core/ReflectionMacros.h"

namespace Script {

    /**
     * @brief 게임 입력을 읽습니다. 뷰포트가 입력을 잡고 있지 않으면 눌리지 않은 것으로 보입니다.
     * @ingroup Script
     */
    class FScriptInput final {
    public:
        /**
         * @brief 읽을 입력 상태를 연결합니다.
         * @param[in] Input 입력 상태이며, nullptr이면 연결을 끊습니다.
         */
        static void SetSource(const Core::FInput* Input);

        /**
         * @brief 키가 눌려 있는지 검사합니다.
         * @param[in] Key 검사할 키입니다.
         * @return 눌려 있으면 true입니다.
         */
        UFUNCTION()
        static bool IsDown(Core::EKey Key);

        /**
         * @brief 이번 프레임에 키를 눌렀는지 검사합니다.
         * @param[in] Key 검사할 키입니다.
         * @return 이번 프레임에 눌렀으면 true입니다.
         */
        UFUNCTION()
        static bool WasPressed(Core::EKey Key);

        /**
         * @brief 이번 프레임에 키를 뗐는지 검사합니다.
         * @param[in] Key 검사할 키입니다.
         * @return 이번 프레임에 뗐으면 true입니다.
         */
        UFUNCTION()
        static bool WasReleased(Core::EKey Key);

        /**
         * @brief 마우스 위치를 반환합니다.
         * @return 창 기준 픽셀 위치입니다.
         */
        UFUNCTION()
        static Core::FVector2 GetMousePosition();

        /**
         * @brief 이번 프레임의 마우스 이동량을 반환합니다.
         * @return 픽셀 단위 이동량입니다.
         */
        UFUNCTION()
        static Core::FVector2 GetMouseDelta();

        /**
         * @brief 이번 프레임의 마우스 휠 이동량을 반환합니다.
         * @return 휠 이동량입니다.
         */
        UFUNCTION()
        static float GetMouseWheel();
    };

}
