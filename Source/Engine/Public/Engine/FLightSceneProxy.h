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
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FLightSceneProxy);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FLightSceneProxy);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

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
        ReflectProperty(mId, "Id", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 환경광·방향광·점광원·스포트라이트 중 광원의 종류입니다.
         */
        Engine::ELightType mType{Engine::ELightType::Ambient};
        ReflectProperty(mType, "Type", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원의 월드 공간 위치입니다.
         */
        Core::FVector mPosition{};
        ReflectProperty(mPosition, "Position", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원이 비추는 월드 공간 방향입니다.
         */
        Core::FVector mDirection{1.0f, 0.0f, 0.0f};
        ReflectProperty(mDirection, "Direction", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원의 RGBA 색상입니다.
         */
        Core::FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
        ReflectProperty(mColor, "Color", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원의 밝기 배율입니다.
         */
        float mIntensity{1.0f};
        ReflectProperty(mIntensity, "Intensity", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 점광원·스포트라이트의 영향 반경입니다.
         */
        float mAttenuationRadius{10.0f};
        ReflectProperty(mAttenuationRadius, "AttenuationRadius", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 거리에 따른 광원 감쇠 지수입니다.
         */
        float mLightFalloffExponent{2.0f};
        ReflectProperty(mLightFalloffExponent, "LightFalloffExponent", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 스포트라이트 내부 원뿔의 반각(도)입니다.
         */
        float mInnerConeAngle{20.0f};
        ReflectProperty(mInnerConeAngle, "InnerConeAngle", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 스포트라이트 외부 원뿔의 반각(도)입니다.
         */
        float mOuterConeAngle{35.0f};
        ReflectProperty(mOuterConeAngle, "OuterConeAngle", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원의 표시 여부입니다.
         */
        bool mVisible{true};
        ReflectProperty(mVisible, "Visible", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 에디터에 표시할 광원 아이콘 텍스처의 비소유 주소입니다.
         */
        Asset::UTexture2D* mIcon{};
        ReflectProperty(mIcon, "Icon", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원의 그림자 생성 여부입니다.
         */
        bool mCastShadows{true};
        ReflectProperty(mCastShadows, "CastShadows", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 기본 그림자 맵의 한 변 해상도입니다.
         */
        Core::Uint32 mShadowResolution{1024};
        ReflectProperty(mShadowResolution, "ShadowResolution", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자 해상도 배율입니다.
         */
        float mShadowResolutionScale{1.0f};
        ReflectProperty(mShadowResolutionScale, "ShadowResolutionScale", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자 깊이 비교의 상수 바이어스입니다.
         */
        float mShadowBias{0.0005f};
        ReflectProperty(mShadowBias, "ShadowBias", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 표면 기울기에 따른 그림자 바이어스 배율입니다.
         */
        float mShadowSlopeBias{1.0f};
        ReflectProperty(mShadowSlopeBias, "ShadowSlopeBias", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자 경계의 선명도 조절값입니다.
         */
        float mShadowSharpen{};
        ReflectProperty(mShadowSharpen, "ShadowSharpen", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 방향광 그림자의 캐스케이드 개수입니다.
         */
        Core::Uint32 mCascadeCount{4};
        ReflectProperty(mCascadeCount, "CascadeCount", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 방향광 그림자의 최대 표시 거리입니다.
         */
        float mShadowDistance{100.0f};
        ReflectProperty(mShadowDistance, "ShadowDistance", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 각 캐스케이드 구간에서 전환에 사용하는 비율입니다.
         */
        float mCascadeTransitionFraction{0.1f};
        ReflectProperty(mCascadeTransitionFraction, "CascadeTransitionFraction", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
