/**
 * @file UPointLightComponent.h
 * @brief 반경과 감쇠 지수를 갖는 점광원을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/ULightComponent.h"

/**
 * @brief 반경과 감쇠 지수를 갖는 점광원을 관리합니다.
 * @ingroup Engine
 */
class UPointLightComponent : public ULightComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UPointLightComponent, ULightComponent);

public:
    /**
     * @brief UPointLightComponent의 기본 상태를 초기화합니다.
     */
    UPointLightComponent();

    /**
     * @brief UPointLightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    ~UPointLightComponent() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 클래스 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UPointLightComponent);

    /**
     * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
     * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
     */
    FLightSceneProxy CreateLightProxy() const override;

    /**
     * @brief 점광원·스포트라이트의 영향 반경을 반환합니다.
     * @return 점광원·스포트라이트의 영향 반경입니다.
     */
    float GetAttenuationRadius() const;

    /**
     * @brief 점광원·스포트라이트의 영향 반경을 검증하고 변경합니다.
     * @param[in] AttenuationRadius 0보다 큰 유한한 광원 영향 반경입니다.
     * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
     */
    bool SetAttenuationRadius(float AttenuationRadius);

    /**
     * @brief 거리에 따른 광원 감쇠 지수를 반환합니다.
     * @return 거리에 따른 광원 감쇠 지수입니다.
     */
    float GetLightFalloffExponent() const;

    /**
     * @brief 거리에 따른 광원 감쇠 지수를 검증하고 변경합니다.
     * @param[in] LightFalloffExponent 0 이상인 유한한 감쇠 지수입니다.
     * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
     */
    bool SetLightFalloffExponent(float LightFalloffExponent);

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 점광원·스포트라이트의 영향 반경입니다.
     */
    float mAttenuationRadius{10.0f};
    ReflectAccessor(mAttenuationRadius, GetAttenuationRadius, SetAttenuationRadius, "AttenuationRadius", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 거리에 따른 광원 감쇠 지수입니다.
     */
    float mLightFalloffExponent{2.0f};
    ReflectAccessor(mLightFalloffExponent, GetLightFalloffExponent, SetLightFalloffExponent, "LightFalloffExponent", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
