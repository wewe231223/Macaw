/**
 * @file FDrawLine.h
 * @brief 시작점, 끝점, 색상과 깊이 검사 정책을 갖는 보조 선분입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

namespace Engine {

    /**
     * @brief 시작점, 끝점, 색상과 깊이 검사 정책을 갖는 보조 선분입니다.
     * @ingroup Engine
     */
    struct FDrawLine {
    public:
        /**
         * @brief 선분 시작점입니다.
         */
        Core::FVector mStart{};

        /**
         * @brief 선분 끝점입니다.
         */
        Core::FVector mEnd{};

        /**
         * @brief 표시 색상입니다.
         */
        Core::FVector mColor{1.0f, 1.0f, 1.0f};

        /**
         * @brief 깊이 검사를 사용할지 여부입니다.
         */
        bool mDepthTest{};

        /**
         * @brief 선분의 픽셀 두께입니다.
         */
        float mThickness{1.0f};
    };

}
