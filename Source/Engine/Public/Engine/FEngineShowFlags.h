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
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FEngineShowFlags);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
         */
        ReflectStruct(FEngineShowFlags);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 현재 뷰포트의 Scene Color에 높이 안개 후처리를 적용할지 나타냅니다.
         */
        bool mFog{true};
        ReflectProperty(mFog, "Fog", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 뷰포트의 Scene Color에 FXAA를 적용할지 나타냅니다.
         */
        bool mFXAA{true};
        ReflectProperty(mFXAA, "FXAA", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 효과의 활성 여부와 독립적으로 에디터의 안개 아이콘을 표시할지 나타냅니다.
         */
        bool mFogIcons{true};
        ReflectProperty(mFogIcons, "FogIcons", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 모든 장면 프리미티브의 표시 여부입니다.
         */
        bool mLightIcons{true};
        ReflectProperty(mLightIcons, "LightIcons", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 뷰에서 그림자 렌더링과 샘플링을 사용할지 나타냅니다.
         */
        bool mShadows{true};
        ReflectProperty(mShadows, "Shadows", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원 방향·반경·원뿔 보조선의 표시 여부입니다.
         */
        bool mLightVisualizers{true};
        ReflectProperty(mLightVisualizers, "LightVisualizers", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 장면 프리미티브의 렌더링 여부입니다.
         */
        bool mPrimitives{true};
        ReflectProperty(mPrimitives, "Primitives", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 스프라이트와 Billboard의 표시 여부입니다.
         */
        bool mSprites{true};
        ReflectProperty(mSprites, "Sprites", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 장면 텍스트와 에디터 이름 표식의 표시 여부입니다.
         */
        bool mText{true};
        ReflectProperty(mText, "Text", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드 AABB 표시 여부입니다.
         */
        bool mBounds{};
        ReflectProperty(mBounds, "Bounds", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드 그리드 표시 여부입니다.
         */
        bool mGrid{true};
        ReflectProperty(mGrid, "Grid", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드 축 표시 여부입니다.
         */
        bool mAxes{true};
        ReflectProperty(mAxes, "Axes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
