/**
 * @file FHeightFogSceneProxy.h
 * @brief 컴포넌트에서 렌더 장면으로 복사하는 안개 설정과 에디터 아이콘의 비소유 참조입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Guid.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"

namespace Asset {
    class UTexture2D;
}

namespace Engine {

    struct FPrimitiveSceneProxy;

    /**
     * @brief 컴포넌트에서 렌더 장면으로 복사하는 안개 설정과 에디터 아이콘의 비소유 참조입니다.
     */
    struct FHeightFogSceneProxy {
    public:
        /**
         * @brief 안개 위치와 아이콘으로 카메라를 향하는 에디터 빌보드 프록시를 구성합니다.
         * @return 안개 GUID와 위치·단위 크기·흰색 아이콘을 가진 프리미티브 프록시입니다.
         */
        Engine::FPrimitiveSceneProxy CreateIconProxy() const;

    public:
        /**
         * @brief 원본 안개 컴포넌트의 GUID이며 장면 프록시 갱신과 제거의 키입니다.
         */
        Core::FGuid mId{};

        /**
         * @brief 안개 액터의 월드 위치이며 Z를 밀도의 기준 높이로 사용합니다.
         */
        Core::FVector mPosition{};

        /**
         * @brief 흡수된 장면 색상에 합성할 기본 산란 RGBA 색상입니다.
         */
        Core::FVector4 mInscatteringColor{0.65f, 0.75f, 0.85f, 1.0f};

        /**
         * @brief 기준 높이에서의 거리당 안개 밀도입니다.
         */
        float mDensity{0.02f};

        /**
         * @brief 기준 높이보다 높아질수록 밀도가 지수적으로 감소하는 계수입니다.
         */
        float mHeightFalloff{0.2f};

        /**
         * @brief 카메라부터 안개 적분을 시작하는 월드 거리입니다.
         */
        float mStartDistance{};

        /**
         * @brief 안개 적분을 종료하는 월드 거리이며 0이면 거리 제한을 두지 않습니다.
         */
        float mEndDistance{1000.0f};

        /**
         * @brief 이 거리보다 먼 표면에는 안개를 적용하지 않으며 0이면 제한이 없습니다.
         */
        float mCutoffDistance{};

        /**
         * @brief 안개 합성 불투명도의 상한이며 0~1 범위입니다.
         */
        float mMaxOpacity{1.0f};

        /**
         * @brief 안개 효과를 렌더 장면에서 사용할지 나타내며 에디터 아이콘 표시와 독립적입니다.
         */
        bool mVisible{true};

        /**
         * @brief 프로젝트 에셋 또는 대체 텍스처가 소유하는 언리얼 안개 아이콘의 비소유 주소입니다.
         */
        Asset::UTexture2D* mIcon{};
    };

}
