/**
 * @file EEditorViewportType.h
 * @brief 원근·정사영 뷰 방향을 선택합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EEditorViewportType.generated.h"

namespace Editor {

    /**
     * @brief 원근 및 여섯 정사영 카메라 방향입니다.
     */
    UENUM()
    enum class EEditorViewportType : Core::Uint8 {
        /**
         * @brief 자유 이동하는 원근 카메라 뷰입니다.
         */
        Perspective,
        /**
         * @brief 위에서 내려다보는 정사영 뷰입니다.
         */
        Top,
        /**
         * @brief 아래에서 올려다보는 정사영 뷰입니다.
         */
        Bottom,
        /**
         * @brief 정면 정사영 뷰입니다.
         */
        Front,
        /**
         * @brief 후면 정사영 뷰입니다.
         */
        Back,
        /**
         * @brief 좌측 정사영 뷰입니다.
         */
        Left,
        /**
         * @brief 우측 정사영 뷰입니다.
         */
        Right
    };

}
