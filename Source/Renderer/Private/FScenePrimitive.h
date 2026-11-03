/**
 * @file FScenePrimitive.h
 * @brief 렌더 장면의 프리미티브 프록시와 GPU 객체 상수 버퍼입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "FObjectConstants.h"
#include "RenderCore/TConstantBuffer.h"
#include "RenderCore/TStructuredBuffer.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <array>

namespace Renderer {

    /**
     * @brief 렌더 장면의 프리미티브 프록시와 GPU 객체 상수 버퍼입니다.
     * @ingroup Renderer
     */
    struct FScenePrimitive {
    public:
        /**
         * @brief 한 원소가 스키닝 행렬과 법선용 역전치 행렬인 본 팔레트 버퍼입니다.
         */
        using FSkinBuffer = RenderCore::TStructuredBuffer<std::array<Core::FMatrix, 2>>;

        /**
         * @brief 엔진에서 복사한 프리미티브 프록시입니다.
         */
        Engine::FPrimitiveSceneProxy mProxy{};

        /**
         * @brief 객체별 GPU 상수 버퍼입니다.
         */
        RenderCore::TConstantBuffer<Renderer::FObjectConstants> mObjectBuffer{};

        /**
         * @brief 프리미티브 드로에 사용하는 월드·UV·그림자 상수의 CPU 사본입니다.
         */
        Renderer::FObjectConstants mObjectConstants{};

        /**
         * @brief 객체별 스키닝 행렬과 법선용 역전치 행렬을 담아 정점 셰이더에서 읽는 구조화 버퍼입니다.
         */
        FSkinBuffer mSkinBuffer{};

        /**
         * @brief 마지막으로 GPU에 업로드한 포즈 개정입니다.
         */
        Core::FRevision mUploadedPoseRevision{};
    };

}
