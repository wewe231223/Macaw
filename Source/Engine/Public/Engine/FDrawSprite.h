/**
 * @file FDrawSprite.h
 * @brief 텍스처 쿼드의 크기·색상·정렬 정보를 전달합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"

namespace Asset {
    class UTexture2D;
}

namespace Engine {

    /**
     * @brief 텍스처 쿼드의 크기, 색상과 카메라 정렬 여부를 전달합니다.
     */
    struct FDrawSprite {
    public:
        /**
         * @brief 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* mTexture{};

        /**
         * @brief 로컬 쿼드의 가로·세로 월드 크기입니다.
         */
        Core::FVector2 mSize{2.0f, 2.0f};

        /**
         * @brief 표시할 RGBA 색상입니다.
         */
        Core::FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};

        /**
         * @brief 카메라의 오른쪽·위쪽 축에 쿼드를 맞출지 여부입니다.
         */
        bool mScreenAligned{true};
    };

}
