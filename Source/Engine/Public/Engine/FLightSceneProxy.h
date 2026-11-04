/**
 * @file FLightSceneProxy.h
 * @brief 렌더 장면에 복사하는 광원 상태와 에디터 아이콘의 비소유 참조입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Guid.h"
#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"
#include "Engine/ELightType.h"

namespace Asset {
    class UTexture2D;
}

namespace Engine {

    struct FPrimitiveSceneProxy;

    /**
     * @brief 렌더 장면에 복사하는 광원 상태와 에디터 아이콘의 비소유 참조입니다.
     * @ingroup Engine
     */
    struct FLightSceneProxy {
    public:
        /**
         * @brief 광원 아이콘을 에디터 빌보드 프리미티브로 구성합니다.
         * @return 광원 GUID와 위치·아이콘을 가진 프리미티브 프록시입니다.
         */
        Engine::FPrimitiveSceneProxy CreateIconProxy() const;

        /**
         * @brief 원본 광원 컴포넌트의 GUID입니다.
         */
        Core::FGuid mId{};

        /**
         * @brief 환경광·방향광·점광원·스포트라이트 중 광원의 종류입니다.
         */
        Engine::ELightType mType{Engine::ELightType::Ambient};

        /**
         * @brief 광원의 월드 공간 위치입니다.
         */
        Core::FVector mPosition{};

        /**
         * @brief 광원이 비추는 월드 공간 방향입니다.
         */
        Core::FVector mDirection{1.0f, 0.0f, 0.0f};

        /**
         * @brief 광원의 RGBA 색상입니다.
         */
        Core::FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};

        /**
         * @brief 광원의 밝기 배율입니다.
         */
        float mIntensity{1.0f};

        /**
         * @brief 점광원·스포트라이트의 영향 반경입니다.
         */
        float mAttenuationRadius{10.0f};

        /**
         * @brief 거리에 따른 광원 감쇠 지수입니다.
         */
        float mLightFalloffExponent{2.0f};

        /**
         * @brief 스포트라이트 내부 원뿔의 반각(도)입니다.
         */
        float mInnerConeAngle{20.0f};

        /**
         * @brief 스포트라이트 외부 원뿔의 반각(도)입니다.
         */
        float mOuterConeAngle{35.0f};

        /**
         * @brief 광원의 표시 여부입니다.
         */
        bool mVisible{true};

        /**
         * @brief 에디터에 표시할 광원 아이콘 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* mIcon{};

        /**
         * @brief 광원의 그림자 생성 여부입니다.
         */
        bool mCastShadows{true};

        /**
         * @brief 기본 그림자 맵의 한 변 해상도입니다.
         */
        Core::Uint32 mShadowResolution{1024};

        /**
         * @brief 그림자 해상도 배율입니다.
         */
        float mShadowResolutionScale{1.0f};

        /**
         * @brief 그림자 깊이 비교의 상수 바이어스이며 단위는 그림자 맵 텍셀입니다. 값의 의미는 캐스케이드와 맵의 깊이 범위에 좌우되지 않습니다.
         */
        float mShadowBias{2.0f};

        /**
         * @brief 표면 기울기에 따른 그림자 바이어스 배율입니다.
         */
        float mShadowSlopeBias{1.0f};

        /**
         * @brief 그림자 경계의 선명도 조절값입니다.
         */
        float mShadowSharpen{};

        /**
         * @brief PCSS 반그림자 계산에 쓰는 점광원·스포트라이트 광원의 반지름(m)입니다.
         */
        float mShadowSourceRadius{0.1f};

        /**
         * @brief PCSS 반그림자 계산에 쓰는 방향광 광원의 각반지름(도)입니다.
         */
        float mShadowSourceAngle{1.0f};

        /**
         * @brief 방향광 그림자의 캐스케이드 개수입니다.
         */
        Core::Uint32 mCascadeCount{4};

        /**
         * @brief 방향광 그림자의 최대 표시 거리입니다.
         */
        float mShadowDistance{100.0f};

        /**
         * @brief 각 캐스케이드 구간에서 전환에 사용하는 비율입니다.
         */
        float mCascadeTransitionFraction{0.25f};
    };

}
