/**
 * @file FEngineShowFlags.h
 * @brief 뷰포트별 장면·보조 표시 필터를 정의합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"

namespace Engine {

    /**
     * @brief 뷰포트마다 독립적으로 적용하는 장면 및 보조 표시 필터입니다.
     */
    struct FEngineShowFlags {
    public:
        /**
         * @brief 현재 뷰포트의 Scene Color에 높이 안개 후처리를 적용할지 나타냅니다.
         */
        bool mFog{true};

        /**
         * @brief 원근 뷰의 빈 배경에 기본 하늘을 그릴지 나타냅니다.
         */
        bool mSky{true};

        /**
         * @brief 현재 뷰포트의 Scene Color에 FXAA를 적용할지 나타냅니다.
         */
        bool mFXAA{true};

        /**
         * @brief 효과의 활성 여부와 독립적으로 에디터의 안개 아이콘을 표시할지 나타냅니다.
         */
        bool mFogIcons{true};

        /**
         * @brief 모든 장면 프리미티브의 표시 여부입니다.
         */
        bool mLightIcons{true};

        /**
         * @brief 현재 뷰에서 그림자 렌더링과 샘플링을 사용할지 나타냅니다.
         */
        bool mShadows{true};

        /**
         * @brief 광원 방향·반경·원뿔 보조선의 표시 여부입니다.
         */
        bool mLightVisualizers{true};

        /**
         * @brief 장면 프리미티브의 렌더링 여부입니다.
         */
        bool mPrimitives{true};

        /**
         * @brief 스프라이트와 Billboard의 표시 여부입니다.
         */
        bool mSprites{true};

        /**
         * @brief 장면 텍스트와 에디터 이름 표식의 표시 여부입니다.
         */
        bool mText{true};

        /**
         * @brief 월드 AABB 표시 여부입니다.
         */
        bool mBounds{};

        /**
         * @brief 월드 그리드 표시 여부입니다.
         */
        bool mGrid{true};

        /**
         * @brief 월드 축 표시 여부입니다.
         */
        bool mAxes{true};
    };

}
