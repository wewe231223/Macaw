/**
 * @file FLineVertex.h
 * @brief 픽셀 두께를 가진 선분의 GPU 인스턴스 형식을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Math/FVector4.h"

#include <type_traits>

namespace Engine {
    struct FDrawLine;
}

namespace Renderer {

    /**
     * @brief 한 선분을 화면상의 두 삼각형으로 확장할 48바이트 인스턴스 데이터입니다.
     */
    struct FLineVertex {
    public:
        /**
         * @brief 선분의 월드 끝점·픽셀 두께·색상을 GPU 인스턴스로 변환합니다.
         * @param[in] Line 변환할 선분의 끝점·색상·두께입니다.
         */
        explicit FLineVertex(const Engine::FDrawLine& Line);

    public:
        /**
         * @brief 시작점 XYZ와 픽셀 두께 W입니다.
         */
        Core::FVector4 mStartAndWidth{};

        /**
         * @brief 선분의 끝점입니다.
         */
        Core::FVector4 mEnd{};

        /**
         * @brief 표시할 RGBA 색상입니다.
         */
        Core::FVector4 mColor{};
    };

    static_assert(std::is_standard_layout_v<Renderer::FLineVertex> && std::is_trivially_copyable_v<Renderer::FLineVertex> && sizeof(Renderer::FLineVertex) == 48);

}
