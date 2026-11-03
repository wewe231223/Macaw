/**
 * @file FPassTarget.h
 * @brief 패스가 그릴 색상·깊이 뷰와 래스터 영역을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <d3d11.h>
#include <optional>

namespace Renderer {

    /**
     * @brief 패스가 그릴 색상·깊이 뷰와 래스터 영역입니다.
     */
    struct FPassTarget {
    public:
        /**
         * @brief 색상을 기록할 렌더 타깃 뷰이며, 깊이 전용 패스는 비어 있습니다.
         */
        ID3D11RenderTargetView* mColor{};

        /**
         * @brief 깊이 검사와 기록에 사용할 깊이·스텐실 뷰입니다.
         */
        ID3D11DepthStencilView* mDepth{};

        /**
         * @brief 클립 공간을 타깃 픽셀로 옮기는 뷰포트입니다.
         */
        D3D11_VIEWPORT mViewport{};

        /**
         * @brief 아틀라스 타일처럼 기록 영역을 제한할 시저 사각형이며, 없으면 제한하지 않습니다.
         */
        std::optional<D3D11_RECT> mScissor{};
    };

}
