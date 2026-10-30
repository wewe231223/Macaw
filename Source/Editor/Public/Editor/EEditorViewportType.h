/**
 * @file EEditorViewportType.h
 * @brief 원근·정사영 뷰 방향을 선택합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

namespace Editor {

    /**
     * @brief 원근 및 여섯 정사영 카메라 방향입니다.
     */
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

    /**
     * @brief 해당 열거형의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectEnum(EEditorViewportType, Perspective, Top, Bottom, Front, Back, Left, Right);

    /**
     * @fn UEnum* GetNativeEnum(TNativeEnum<EEditorViewportType>)
     * @brief 타입 태그로 뷰 방향의 열거형 메타데이터를 조회합니다.
     * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
     */

}
