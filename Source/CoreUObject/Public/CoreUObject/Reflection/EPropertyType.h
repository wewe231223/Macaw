/**
 * @file EPropertyType.h
 * @brief 프로퍼티 값의 저장 및 직렬화에 지원하는 타입을 구분합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/Types.h"

namespace CoreUObject {

    /**
     * @brief 프로퍼티 값의 저장 및 직렬화에 지원하는 타입을 구분합니다.
     * @ingroup CoreUObject
     */
    enum class EPropertyType {
        /**
         * @brief bool 값입니다.
         */
        Boolean,
        /**
         * @brief Int32 부호 있는 정수입니다.
         */
        Integer,
        /**
         * @brief Uint32 부호 없는 정수이며 Uint8의 저장에도 사용합니다.
         */
        Unsigned,
        /**
         * @brief Uint64 부호 없는 정수입니다.
         */
        Unsigned64,
        /**
         * @brief 단정밀도 실수입니다.
         */
        Float,
        /**
         * @brief UTF-8 문자열입니다.
         */
        String,
        /**
         * @brief 128비트 GUID입니다.
         */
        Guid,
        /**
         * @brief 2차원 벡터입니다.
         */
        Vector2,
        /**
         * @brief 3차원 벡터입니다.
         */
        Vector,
        /**
         * @brief 4차원 벡터입니다.
         */
        Vector4,
        /**
         * @brief 쿼터니언 회전입니다.
         */
        Quat,
        /**
         * @brief 도 단위 오일러 회전입니다.
         */
        Rotator,
        /**
         * @brief 4×4 행렬입니다.
         */
        Matrix,
        /**
         * @brief 위치, 회전과 스케일을 묶은 변환입니다.
         */
        Transform,
        /**
         * @brief 축 정렬 경계 상자입니다.
         */
        Box,
        /**
         * @brief 원점과 방향으로 정의한 광선입니다.
         */
        Ray,
        /**
         * @brief 열거형 메타데이터 또는 Int32로 저장하는 열거값입니다.
         */
        Enum,
        /**
         * @brief 일반 variant 값으로 변환할 수 없는 네이티브 타입입니다.
         */
        Opaque
    };

}
