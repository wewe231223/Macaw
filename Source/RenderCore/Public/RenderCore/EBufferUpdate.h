/**
 * @file EBufferUpdate.h
 * @brief GPU 버퍼의 CPU 갱신 방식을 선언합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"

namespace RenderCore {

    /**
     * @brief CPU 데이터를 GPU 버퍼에 기록하는 방식을 나타냅니다.
     */
    enum class EBufferUpdate : Core::Uint8 {
        /**
         * @brief D3D11_USAGE_DEFAULT 버퍼를 UpdateSubresource로 기록합니다.
         */
        Default,
        /**
         * @brief D3D11_USAGE_DYNAMIC 버퍼를 WRITE_DISCARD 매핑으로 기록합니다.
         */
        Dynamic,
        /**
         * @brief D3D11_USAGE_IMMUTABLE 버퍼를 생성할 때 한 번만 기록합니다.
         */
        Immutable
    };

}
