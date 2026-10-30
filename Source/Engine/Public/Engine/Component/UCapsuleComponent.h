/**
 * @file UCapsuleComponent.h
 * @brief Z축 중심 선분과 반구를 가진 캡슐 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UShapeComponent.h"

/**
 * @brief Z축 중심 선분과 반구를 가진 캡슐 충돌 컴포넌트입니다.
 */
class UCapsuleComponent : public UShapeComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UCapsuleComponent, UShapeComponent);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectClass(UCapsuleComponent);

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
    void Serialize(FArchive& Archive) override;

    /**
     * @brief 캡슐의 로컬 반지름과 반높이를 함께 설정합니다.
     * @param[in] InRadius 형상의 반지름입니다.
     * @param[in] InHalfHeight 반구를 포함한 캡슐 반높이입니다.
     */
    void SetCapsuleSize(float InRadius, float InHalfHeight);

    /**
     * @brief 월드 스케일을 반영한 캡슐 반지름을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetScaledCapsuleRadius() const;

    /**
     * @brief 월드 스케일을 반영한 캡슐 반높이를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetScaledCapsuleHalfHeight() const;

    /**
     * @brief 반구를 제외한 캡슐 중심 선분 반길이를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetScaledCapsuleHalfHeightWithoutHemisphere() const;

    /**
     * @brief 반구를 포함한 캡슐 반높이를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetCapsuleHalfHeight() const;

    /**
     * @brief 반구를 포함한 캡슐 반높이를 설정합니다.
     * @param[in] InHalfHeight 반구를 포함한 캡슐 반높이입니다.
     */
    void SetCapsuleHalfHeight(float InHalfHeight);

    /**
     * @brief 캡슐의 로컬 반지름을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetCapsuleRadius() const;

    /**
     * @brief 캡슐의 로컬 반지름을 설정합니다.
     * @param[in] InRadius 형상의 반지름입니다.
     */
    void SetCapsuleRadius(float InRadius);

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    // 반구를 포함한 높이
    /**
     * @brief 반구를 포함한 캡슐 반높이입니다.
     */
    float mCapsuleHalfHeight{1.0f};
    ReflectAccessor(mCapsuleHalfHeight, GetCapsuleHalfHeight, SetCapsuleHalfHeight, "CapsuleHalfHeight", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 캡슐의 로컬 반지름입니다.
     */
    float mCapsuleRadius{0.5f};
    ReflectAccessor(mCapsuleRadius, GetCapsuleRadius, SetCapsuleRadius, "CapsuleRadius", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
