/**
 * @file ULightComponent.h
 * @brief 광원의 그림자 생성 여부와 해상도·바이어스 설정을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/ULightComponentBase.h"

namespace Engine {

    /**
     * @brief 광원의 그림자 생성 여부와 해상도·바이어스 설정을 관리합니다.
     * @ingroup Engine
     */
    class ULightComponent : public Engine::ULightComponentBase {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(ULightComponent, Engine::ULightComponentBase);

    public:
        /**
         * @brief ULightComponent의 기본 상태를 초기화합니다.
         */
        ULightComponent();

        /**
         * @brief ULightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
         */
        ~ULightComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
         * @return 등록된 클래스 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(ULightComponent, CoreUObject::EClassFlags::NoFactory);

        /**
         * @brief 광원의 그림자 생성 여부를 반환합니다.
         * @return 광원의 그림자 생성 여부입니다.
         */
        bool GetCastShadows() const;

        /**
         * @brief 광원의 그림자 생성 여부를 검증하고 변경합니다.
         * @param[in] CastShadows 광원이 그림자를 생성하도록 설정하려면 true입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetCastShadows(bool CastShadows);

        /**
         * @brief 기본 그림자 맵의 한 변 해상도를 반환합니다.
         * @return 기본 그림자 맵의 한 변 해상도입니다.
         */
        Core::Uint32 GetShadowResolution() const;

        /**
         * @brief 기본 그림자 맵의 한 변 해상도를 검증하고 변경합니다.
         * @param[in] Resolution 64 이상 8192 이하인 2의 거듭제곱 해상도입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowResolution(Core::Uint32 Resolution);

        /**
         * @brief 그림자 해상도 배율을 반환합니다.
         * @return 그림자 해상도 배율입니다.
         */
        float GetShadowResolutionScale() const;

        /**
         * @brief 그림자 해상도 배율을 검증하고 변경합니다.
         * @param[in] Scale 0.125 이상 8 이하인 유한한 해상도 배율입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowResolutionScale(float Scale);

        /**
         * @brief 그림자 깊이 비교의 상수 바이어스를 반환합니다.
         * @return 그림자 깊이 비교의 상수 바이어스입니다.
         */
        float GetShadowBias() const;

        /**
         * @brief 그림자 깊이 비교의 상수 바이어스를 검증하고 변경합니다.
         * @param[in] Bias 0 이상 0.1 이하인 유한한 깊이 바이어스입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowBias(float Bias);

        /**
         * @brief 표면 기울기에 따른 그림자 바이어스 배율을 반환합니다.
         * @return 표면 기울기에 따른 그림자 바이어스 배율입니다.
         */
        float GetShadowSlopeBias() const;

        /**
         * @brief 표면 기울기에 따른 그림자 바이어스 배율을 검증하고 변경합니다.
         * @param[in] Bias 0 이상 16 이하인 유한한 기울기 바이어스입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowSlopeBias(float Bias);

        /**
         * @brief 그림자 경계의 선명도 조절값을 반환합니다.
         * @return 그림자 경계의 선명도 조절값입니다.
         */
        float GetShadowSharpen() const;

        /**
         * @brief 그림자 경계의 선명도 조절값을 검증하고 변경합니다.
         * @param[in] Sharpen 0 이상 10 이하인 유한한 경계 선명도입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowSharpen(float Sharpen);

        /**
         * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
         * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
         */
        Engine::FLightSceneProxy CreateLightProxy() const override;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 광원의 그림자 생성 여부입니다.
         */
        bool mCastShadows{true};
        ReflectAccessor(mCastShadows, GetCastShadows, SetCastShadows, "CastShadows", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 기본 그림자 맵의 한 변 해상도입니다.
         */
        Core::Uint32 mShadowResolution{1024};
        ReflectAccessor(mShadowResolution, GetShadowResolution, SetShadowResolution, "ShadowResolution", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 그림자 해상도 배율입니다.
         */
        float mShadowResolutionScale{1.0f};
        ReflectAccessor(mShadowResolutionScale, GetShadowResolutionScale, SetShadowResolutionScale, "ShadowResolutionScale", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 그림자 깊이 비교의 상수 바이어스입니다.
         */
        float mShadowBias{0.0005f};
        ReflectAccessor(mShadowBias, GetShadowBias, SetShadowBias, "ShadowBias", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 표면 기울기에 따른 그림자 바이어스 배율입니다.
         */
        float mShadowSlopeBias{1.0f};
        ReflectAccessor(mShadowSlopeBias, GetShadowSlopeBias, SetShadowSlopeBias, "ShadowSlopeBias", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 그림자 경계의 선명도 조절값입니다.
         */
        float mShadowSharpen{};
        ReflectAccessor(mShadowSharpen, GetShadowSharpen, SetShadowSharpen, "ShadowSharpen", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
    };

}
