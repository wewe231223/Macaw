/**
 * @file FScenePrimitive.h
 * @brief 렌더 장면의 프리미티브 프록시와 GPU 객체 상수 버퍼입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "FObjectConstants.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    /**
     * @brief 렌더 장면의 프리미티브 프록시와 GPU 객체 상수 버퍼입니다.
     * @ingroup Renderer
     */
    struct FScenePrimitive {
    public:
        /**
         * @brief 엔진에서 복사한 프리미티브 프록시입니다.
         */
        Engine::FPrimitiveSceneProxy mProxy{};

        /**
         * @brief 객체별 GPU 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mObjectBuffer{};

        /**
         * @brief 프리미티브 드로에 사용하는 월드·UV·그림자 상수의 CPU 사본입니다.
         */
        Renderer::FObjectConstants mObjectConstants{};

        /**
         * @brief 객체별 스키닝 행렬과 법선용 역전치 행렬을 담은 구조화 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mSkinBuffer{};

        /**
         * @brief 정점 셰이더에서 객체별 본 행렬을 읽는 SRV입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSkinView{};

        /**
         * @brief 마지막으로 GPU에 업로드한 포즈 개정입니다.
         */
        Core::FRevision mUploadedPoseRevision{};
    };

}
