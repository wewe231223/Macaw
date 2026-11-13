/**
 * @file FMeshPassParameters.h
 * @brief 메시 패스를 한 번 그릴 때마다 달라지는 출력 타깃과 뷰 투영을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "FPassTarget.h"

#include <span>

namespace Renderer {

    /**
     * @brief 메시 패스를 한 번 그릴 때마다 달라지는 출력 타깃과 뷰 투영 묶음입니다.
     */
    struct FMeshPassParameters {
    public:

        /**
         * @brief 패스 결과를 기록할 출력 타깃입니다.
         */
        Renderer::FPassTarget mTarget{};

        /**
         * @brief 호출자가 소유하는 출력 컬러 렌더 타깃 뷰 목록입니다.
         */
        std::span<ID3D11RenderTargetView* const> mColorAttachments{};

        /**
         * @brief 정점 변환과 프러스텀 컬링에 함께 사용하는 월드→클립 행렬입니다.
         */
        Core::FMatrix mViewProjection{};

        /**
         * @brief 패스에 사용하는 래스터라이저 상태입니다.
         */
        ID3D11RasterizerState* mRasterizer{};
    };

}
