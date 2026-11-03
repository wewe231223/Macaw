/**
 * @file FMeshVertex.h
 * @brief CPU 메시 정점의 위치와 표면 법선입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

namespace Asset {

    /**
     * @brief CPU 메시 정점의 위치와 표면 법선입니다.
     * @ingroup Asset
     */
    struct FMeshVertex {
    public:
        /**
         * @brief 정점 위치입니다.
         */
        Core::FVector mPosition{};

        /**
         * @brief 정점 법선입니다.
         */
        Core::FVector mNormal{};
    };

}
