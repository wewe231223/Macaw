/**
 * @file ETextureFormat.h
 * @brief 텍스처 픽셀·깊이 형식과 열거형 리플렉션에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "ETextureFormat.generated.h"

namespace Asset {

    /**
     * @brief 텍스처의 픽셀 및 깊이·스텐실 저장 형식을 구분합니다.
     */
    UENUM()
    enum class ETextureFormat : Core::Uint8 {
        /**
         * @brief 각 RGBA 성분을 8비트 정규화 값으로 저장하는 색상 형식입니다.
         */
        RGBA8,
        /**
         * @brief 각 BGRA 성분을 8비트 정규화 값으로 저장하는 색상 형식입니다.
         */
        BGRA8,
        /**
         * @brief 각 RGBA 성분을 16비트 부동소수점으로 저장합니다.
         */
        RGBA16Float,
        /**
         * @brief 각 RGBA 성분을 32비트 부동소수점으로 저장합니다.
         */
        RGBA32Float,
        /**
         * @brief 단일 R 성분을 32비트 부동소수점으로 저장합니다.
         */
        R32Float,
        /**
         * @brief 24비트 깊이와 8비트 스텐실을 저장합니다.
         */
        D24S8,
        /**
         * @brief 깊이를 32비트 부동소수점으로 저장합니다.
         */
        D32Float
    };

}
