/**
 * @file FShadowSettings.h
 * @brief 콘솔에서 조절하는 그림자 렌더링 옵션을 정의합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace RenderCore {

    /**
     * @brief 그림자 가시성을 계산하는 필터 방식입니다.
     * @details ShadowSampling.hlsli의 SHADOW_FILTER_* 값과 순서가 같아야 합니다.
     * @ingroup RenderCore
     */
    enum class EShadowFilter : Core::Uint32 {
        /**
         * @brief 깊이를 한 번 비교하는 경계가 선명한 그림자 필터입니다.
         */
        Hard,

        /**
         * @brief 주변 깊이 비교 결과를 평균하는 PCF 필터입니다.
         */
        PCF,

        /**
         * @brief 차폐물 거리에 따라 반영 영역을 조절하는 PCSS 필터입니다.
         */
        PCSS
    };

    /**
     * @brief 콘솔에서 조절하는 그림자 렌더링 옵션입니다.
     * @ingroup RenderCore
     */
    struct FShadowSettings {
    public:
        /**
         * @brief 그림자 가시성 필터 방식입니다.
         */
        RenderCore::EShadowFilter mFilter{RenderCore::EShadowFilter::PCF};

        /**
         * @brief PCF 격자 한 변의 탭 수이며 3, 5, 7 중 하나입니다.
         */
        Core::Uint32 mPCFSize{3};

        /**
         * @brief PCSS 필터 반경의 상한(텍셀)입니다.
         */
        float mPCSSMaxFilterTexels{32.0f};

        /**
         * @brief 방향광 캐스케이드 수를 강제하는 값이며 0이면 광원 속성을 따릅니다.
         */
        Core::Uint32 mCascadeOverride{};

        /**
         * @brief 입력이 같은 그림자 타일의 깊이를 재사용할지 여부입니다.
         */
        bool mCacheWholeSceneShadows{true};

        /**
         * @brief 화면에 영향을 주지 않는 광원의 그림자 맵 생성을 건너뛸지 여부입니다.
         */
        bool mSkipUnseenLights{true};
    };

}
