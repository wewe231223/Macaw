/**
 * @file EWorldType.h
 * @brief 월드의 편집 또는 실행 용도를 구분합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

namespace Engine {

    /**
     * @brief 월드의 편집 또는 실행 용도를 구분합니다.
     * @ingroup Engine
     */
    enum class EWorldType : Core::Uint8 {
        /**
         * @brief 원본 장면을 편집하는 월드이며 게임 실행을 시작하지 않습니다.
         */
        Editor,
        /**
         * @brief 게임 생명주기를 실행할 수 있는 독립 게임 월드입니다.
         */
        Game,
        /**
         * @brief 에디터 내부 실행에 사용할 월드이며 게임 생명주기를 실행할 수 있습니다.
         */
        PIE,
        /**
         * @brief 편집 도구의 미리보기 월드이며 게임 실행을 시작하지 않습니다.
         */
        EditorPreview
    };

    /**
     * @brief 이 열거형의 정적 메타데이터를 반환합니다.
     * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
     */
    ReflectEnum(EWorldType, Editor, Game, PIE, EditorPreview);

    /**
     * @fn UEnum* GetNativeEnum(TNativeEnum<EWorldType>)
     * @brief 타입 태그를 통해 이 열거형의 메타데이터를 조회합니다.
     * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
     */

}
