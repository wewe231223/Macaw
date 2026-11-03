/**
 * @file FGpuMeshVertex.h
 * @brief GPU 입력 레이아웃에 맞춘 24바이트 위치·법선 정점입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <array>
#include <cstddef>
#include <type_traits>

namespace Asset {
    struct FMeshVertex;
}

namespace Renderer {

    /**
     * @brief GPU 입력 레이아웃에 맞춘 24바이트 위치·법선 정점입니다.
     * @ingroup Renderer
     */
    struct FGpuMeshVertex {
    public:
        /**
         * @brief FGpuMeshVertex의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Vertex GPU 레이아웃으로 변환할 CPU 정점입니다.
         */
        explicit FGpuMeshVertex(const Asset::FMeshVertex& Vertex);

    public:
        /**
         * @brief 정점 위치입니다.
         */
        std::array<float, 3> mPosition{};

        /**
         * @brief 정점 법선입니다.
         */
        std::array<float, 3> mNormal{};
    };

    static_assert(std::is_standard_layout_v<Renderer::FGpuMeshVertex> && std::is_trivially_copyable_v<Renderer::FGpuMeshVertex>);
    static_assert(sizeof(Renderer::FGpuMeshVertex) == 24 && offsetof(Renderer::FGpuMeshVertex, mPosition) == 0 && offsetof(Renderer::FGpuMeshVertex, mNormal) == 12);

}
