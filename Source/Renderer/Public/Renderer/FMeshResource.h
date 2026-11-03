/**
 * @file FMeshResource.h
 * @brief GPU 정점·인덱스 버퍼와 인덱스 개수 및 메모리 크기입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    /**
     * @brief GPU 정점·인덱스 버퍼와 인덱스 개수 및 메모리 크기입니다.
     * @ingroup Renderer
     */
    struct FMeshResource {
    public:
        /**
         * @brief GPU 정점 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mVertices{};

        /**
         * @brief GPU 인덱스 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mIndices{};

        /**
         * @brief 드로 호출에 사용할 인덱스 개수입니다.
         */
        Core::Uint32 mIndexCount{};

        /**
         * @brief GPU 버퍼가 차지하는 바이트 수입니다.
         */
        std::size_t mBytes{};
    };

}
