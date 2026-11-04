/**
 * @file FRenderStats.h
 * @brief 프레임 드로 호출, 가시 프리미티브, 변환 업로드와 GPU 메모리 통계입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace RenderCore {

    /**
     * @brief 프레임 드로 호출, 가시 프리미티브, 변환 업로드와 GPU 메모리 통계입니다.
     * @ingroup RenderCore
     */
    struct FRenderStats {
    public:
        /**
         * @brief 프레임에서 제출한 드로 호출 수입니다.
         */
        Core::Uint32 mDrawCalls{};

        /**
         * @brief 프레임에서 렌더링한 프리미티브 수입니다.
         */
        Core::Uint32 mVisiblePrimitives{};

        /**
         * @brief 소비 전 누적된 객체 상수 버퍼 갱신 수입니다.
         */
        Core::Uint32 mTransformUpdates{};

        /**
         * @brief 집계한 GPU 리소스 바이트 수입니다.
         */
        std::size_t mGpuBytes{};

        /**
         * @brief 현재 뷰에 제출한 가시 방향광 개수입니다.
         */
        Core::Uint32 mDirectionalLights{};

        /**
         * @brief 현재 뷰에 제출한 가시 점광원 개수입니다.
         */
        Core::Uint32 mPointLights{};

        /**
         * @brief 현재 뷰에 제출한 가시 스포트라이트 개수입니다.
         */
        Core::Uint32 mSpotLights{};

        /**
         * @brief 현재 뷰에 할당한 그림자 맵 면·캐스케이드의 총개수입니다.
         */
        Core::Uint32 mShadowMaps{};

        /**
         * @brief 그림자 깊이 패스에 제출한 드로 호출 수입니다.
         */
        Core::Uint32 mShadowDrawCalls{};

        /**
         * @brief 아틀라스 공간 부족 등으로 그림자 생성을 생략한 광원 수입니다.
         */
        Core::Uint32 mShadowDroppedLights{};

        /**
         * @brief 공간 확보를 위해 요청 해상도를 줄인 광원 수입니다.
         */
        Core::Uint32 mShadowReducedLights{};

        /**
         * @brief 화면에 영향을 주지 않아 그림자 맵 생성을 건너뛴 광원 수입니다.
         */
        Core::Uint32 mShadowSkippedLights{};

        /**
         * @brief 캐시된 깊이를 재사용해 다시 그리지 않은 그림자 타일 수입니다.
         */
        Core::Uint32 mShadowCachedTiles{};

        /**
         * @brief 그림자 깊이 아틀라스의 전체 바이트 수입니다.
         */
        std::size_t mShadowAtlasBytes{};

        /**
         * @brief 보호 여백을 포함해 그림자 타일에 할당한 바이트 수입니다.
         */
        std::size_t mShadowAllocatedBytes{};
    };

}
