/**
 * @file FEditorMeshConstants.h
 * @brief 편집기 메시 셰이더에 전달하는 월드 행렬과 색상에 대한 선언입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include <array>
#include <cstddef>
#include <type_traits>

namespace Renderer {

    /**
     * @brief 에디터 메시의 변환과 색상을 전달하는 GPU 상수입니다.
     * @ingroup Renderer
     */
    struct FEditorMeshConstants {
    public:
        /**
         * @brief 에디터 메시 정점 셰이더가 로컬 위치를 변환하는 월드 행렬입니다.
         */
        Core::FMatrix mWorld{};

        /**
         * @brief 에디터 메시 픽셀 셰이더에 전달할 RGB 색상과 알파 값입니다.
         */
        std::array<float, 4> mColor{};

        std::array<float, 4> mShading{};
    };

    static_assert(std::is_standard_layout_v<Renderer::FEditorMeshConstants> && std::is_trivially_copyable_v<Renderer::FEditorMeshConstants> && sizeof(Renderer::FEditorMeshConstants) == 96);
    static_assert(offsetof(Renderer::FEditorMeshConstants, mWorld) == 0 && offsetof(Renderer::FEditorMeshConstants, mColor) == 64 && offsetof(Renderer::FEditorMeshConstants, mShading) == 80);

}
