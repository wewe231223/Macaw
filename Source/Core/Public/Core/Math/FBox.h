/**
 * @file FBox.h
 * @brief 최솟값과 최댓값으로 정의하는 축 정렬 경계 상자입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

struct FRay;
struct FMatrix;

/**
 * @brief 최솟값과 최댓값으로 정의하는 축 정렬 경계 상자입니다.
 * @ingroup Core
 */
struct alignas(16) FBox {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FBox);

public:
    /**
     * @brief FBox의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FBox() = default;

    /**
     * @brief FBox의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Minimum 성분별 하한입니다.
     * @param[in] Maximum 성분별 상한입니다.
     */
    FBox(const FVector& Minimum, const FVector& Maximum);

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FBox() = default;

    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FBox(const FBox& Other) = default;

    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FBox& operator=(const FBox& Other) = default;

    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FBox(FBox&& Other) noexcept = default;

    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FBox& operator=(FBox&& Other) noexcept = default;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FBox);

    /**
     * @brief 광선이 최대 거리 이내에서 축 정렬 경계 상자와 교차하는지 검사합니다.
     * @param[in] Ray 교차를 검사하거나 역투영 결과를 기록할 광선입니다.
     * @param[in] MaximumDistance 허용할 최대 광선 거리입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Intersects(const FRay& Ray, float MaximumDistance) const;

    /**
     * @brief 경계 상자의 모서리를 변환한 뒤 새 축 정렬 경계 상자를 계산합니다.
     * @param[in] Matrix 적용할 변환 행렬입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FBox Transformed(const FMatrix& Matrix) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 축별 최솟값입니다.
     */
    FVector mMinimum{};
    ReflectProperty(mMinimum, "Minimum", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 축별 최댓값입니다.
     */
    FVector mMaximum{};
    ReflectProperty(mMaximum, "Maximum", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
