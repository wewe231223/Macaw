/**
 * @file FQuat.h
 * @brief 회전의 합성과 보간에 사용하는 단위 쿼터니언입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <xmmintrin.h>

struct FVector;
struct FRotator;

#pragma warning(push)
#pragma warning(disable: 4201)

/**
 * @brief 회전의 합성과 보간에 사용하는 단위 쿼터니언입니다.
 * @ingroup Core
 */
struct alignas(16) FQuat {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FQuat);

private:
    /**
     * @brief SIMD 저장소를 사용하는 수학 연산 또는 객체 등록을 위해 내부 접근을 허용합니다.
     */
    friend struct FRotator;

public:
    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FQuat();
    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] X X 성분입니다.
     * @param[in] Y Y 성분입니다.
     * @param[in] Z Z 성분입니다.
     * @param[in] W W 성분입니다.
     */
    FQuat(float X, float Y, float Z, float W);
    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Rotation 적용할 오일러 또는 쿼터니언 회전입니다.
     */
    explicit FQuat(const FRotator& Rotation);
    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Axis 회전 또는 편집에 사용할 축입니다.
     * @param[in] Radians 라디안 단위 각도입니다.
     */
    FQuat(const FVector& Axis, float Radians);

private:
    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Value 연산할 SIMD 레지스터 값입니다.
     */
    explicit FQuat(__m128 Value);

public:
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FQuat() = default;
    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FQuat(const FQuat& Other) = default;
    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FQuat& operator=(const FQuat& Other) = default;
    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FQuat(FQuat&& Other) noexcept = default;
    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FQuat& operator=(FQuat&& Other) noexcept = default;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FQuat);

    /**
     * @brief 두 쿼터니언의 회전 합성 곱을 계산합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FQuat operator*(const FQuat& Other) const;
    /**
     * @brief 두 쿼터니언의 회전 합성 곱을 계산합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FQuat& operator*=(const FQuat& Other);
    /**
     * @brief 현재 쿼터니언 회전을 벡터에 적용합니다.
     * @param[in] Vector 연산에 사용할 벡터입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector operator*(const FVector& Vector) const;
    /**
     * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FQuat operator-() const;
    /**
     * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool operator==(const FQuat& Other) const;

    /**
     * @brief 대응 성분의 곱을 합산하여 내적을 계산합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 대응 성분 곱의 합입니다.
     */
    float Dot(const FQuat& Other) const;
    /**
     * @brief 성분의 제곱합을 반환합니다.
     * @return 현재 값의 제곱 길이입니다.
     */
    float LengthSquared() const;
    /**
     * @brief 유클리드 길이를 반환합니다.
     * @return 현재 값의 유클리드 길이입니다.
     */
    float Length() const;
    /**
     * @brief 정규화한 복사본을 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     * @note 정규화할 수 없는 경우 항등 회전을 반환합니다.
     */
    FQuat Normalized() const;
    /**
     * @brief 값을 제자리에서 정규화합니다.
     * @return 정규화할 수 있으면 true이며, 퇴화하거나 유효하지 않은 값이면 false입니다.
     * @note 정규화할 수 없는 경우 항등 회전으로 초기화하고 false를 반환합니다.
     */
    bool Normalize();
    /**
     * @brief 쿼터니언 벡터 성분의 부호를 반전한 켤레를 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FQuat Conjugate() const;
    /**
     * @brief 켤레를 제곱 길이로 나눈 역쿼터니언을 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     * @note 유한하지 않거나 제곱 길이가 너무 작은 경우 영 쿼터니언을 반환합니다.
     */
    FQuat Inverse() const;
    /**
     * @brief 쿼터니언 회전을 도 단위 오일러 각으로 변환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator Rotator() const;

    /**
     * @brief 최단 회전 경로를 선택하여 선형 보간한 뒤 정규화합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @param[in] Alpha 보간 비율입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FQuat Nlerp(const FQuat& Other, float Alpha) const;
    /**
     * @brief 최단 회전 경로를 따라 구면 선형 보간합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @param[in] Alpha 보간 비율입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FQuat Slerp(const FQuat& Other, float Alpha) const;
    /**
     * @brief 두 쿼터니언 회전 사이의 각도 차이를 라디안으로 구합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 최단 회전 경로의 라디안 각도입니다.
     */
    float AngularDistance(const FQuat& Other) const;
    /**
     * @brief 쿼터니언 회전을 단위 축과 라디안 각도로 분해합니다.
     * @param[in,out] Axis 회전 또는 편집에 사용할 축입니다.
     * @param[in,out] Radians 라디안 단위 각도입니다.
     */
    void ToAxisAngle(FVector& Axis, float& Radians) const;

    /**
     * @brief 현재 회전을 벡터에 적용합니다.
     * @param[in] Vector 연산에 사용할 벡터입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector RotateVector(const FVector& Vector) const;
    /**
     * @brief 현재 회전의 역회전을 벡터에 적용합니다.
     * @param[in] Vector 연산에 사용할 벡터입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector UnrotateVector(const FVector& Vector) const;
    /**
     * @brief 회전된 전방 단위 벡터를 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector ForwardVector() const;
    /**
     * @brief 회전된 우측 단위 벡터를 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector RightVector() const;
    /**
     * @brief 회전된 상방 단위 벡터를 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector UpVector() const;

    /**
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsFinite() const;
    /**
     * @brief 제곱 길이가 허용 오차 이내에서 1인지 검사합니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsNormalized(float Tolerance = 0.0001f) const;
    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Equals(const FQuat& Other, float Tolerance = 0.0001f) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief SIMD 레지스터와 개별 성분을 같은 메모리에 배치합니다.
     */
    union {
        /**
         * @brief 저장된 값입니다.
         */
        __m128 mValue{};
        ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

        /**
         * @brief SIMD 저장소와 겹쳐 배치한 개별 스칼라 성분입니다.
         */
        struct {
            /**
             * @brief X 성분입니다.
             */
            float x;
            ReflectProperty(x, "X", EPropertyFlags::Edit | EPropertyFlags::Serialize);

            /**
             * @brief Y 성분입니다.
             */
            float y;
            ReflectProperty(y, "Y", EPropertyFlags::Edit | EPropertyFlags::Serialize);

            /**
             * @brief Z 성분입니다.
             */
            float z;
            ReflectProperty(z, "Z", EPropertyFlags::Edit | EPropertyFlags::Serialize);

            /**
             * @brief 동차 좌표 또는 쿼터니언의 W 성분입니다.
             */
            float w;
            ReflectProperty(w, "W", EPropertyFlags::Edit | EPropertyFlags::Serialize);
        };
    };
};

#pragma warning(pop)

static_assert(sizeof(FQuat) == 16 && alignof(FQuat) == 16);
