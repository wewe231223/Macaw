/**
 * @file USphereComponent.h
 * @brief 로컬 반지름으로 정의한 구 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UShapeComponent.h"

/**
 * @brief 로컬 반지름으로 정의한 구 충돌 컴포넌트입니다.
 */
class USphereComponent : public UShapeComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(USphereComponent, UShapeComponent);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectClass(USphereComponent);

    /**
     * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FCollisionShape GetCollisionShape() const override;

    /**
     * @brief 충돌 형상의 로컬 축 정렬 경계를 계산합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FBox CalcLocalBounds() const override;

    /**
     * @brief 구의 로컬 반지름을 설정합니다.
     * @param[in] InRadius 형상의 반지름입니다.
     */
    void SetSphereRadius(float InRadius);

    /**
     * @brief 월드 스케일을 반영한 구 반지름을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetScaledSphereRadius() const;

    /**
     * @brief 구의 로컬 반지름을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetSphereRadius() const;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 구의 로컬 반지름입니다.
     */
    float mSphereRadius{0.5f};
    ReflectAccessor(mSphereRadius, GetSphereRadius, SetSphereRadius, "SphereRadius", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
