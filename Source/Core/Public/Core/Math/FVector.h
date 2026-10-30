/**
 * @file FVector.h
 * @brief 16바이트 정렬 SIMD 저장소를 사용하는 3차원 벡터입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <xmmintrin.h>

struct FRotator;

#pragma warning(push)
#pragma warning(disable: 4201)

/**
 * @brief 16바이트 정렬 SIMD 저장소를 사용하는 3차원 벡터입니다.
 * @ingroup Core
 */
struct alignas(16) FVector {
private:
    /**
     * @brief SIMD 저장소를 사용하는 수학 연산 또는 객체 등록을 위해 내부 접근을 허용합니다.
     */
    friend struct FBox;

    /**
     * @brief SIMD 저장소를 사용하는 수학 연산 또는 객체 등록을 위해 내부 접근을 허용합니다.
     */
    friend struct FMatrix;

    /**
     * @brief SIMD 저장소를 사용하는 수학 연산 또는 객체 등록을 위해 내부 접근을 허용합니다.
     */
    friend struct FQuat;

public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FVector);

public:
    /**
     * @brief FVector의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FVector() = default;

    /**
     * @brief FVector의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] X X 성분입니다.
     * @param[in] Y Y 성분입니다.
     * @param[in] Z Z 성분입니다.
     */
    FVector(float X, float Y, float Z);

private:
    /**
     * @brief FVector의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Value 연산할 SIMD 레지스터 값입니다.
     */
    explicit FVector(__m128 Value);

public:
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FVector() = default;

    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FVector(const FVector& Other) = default;

    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FVector& operator=(const FVector& Other) = default;

    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FVector(FVector&& Other) noexcept = default;

    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FVector& operator=(FVector&& Other) noexcept = default;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FVector);

    /**
     * @brief 대응 성분을 더한 값을 반환합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector operator+(const FVector& Other) const;

    /**
     * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector operator-(const FVector& Other) const;

    /**
     * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector operator-() const;

    /**
     * @brief 대응하는 성분끼리 곱합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector operator*(const FVector& Other) const;

    /**
     * @brief 성분별 또는 스칼라 나눗셈 결과를 반환합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector operator/(const FVector& Other) const;

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector operator*(float Value) const;

    /**
     * @brief 성분별 또는 스칼라 나눗셈 결과를 반환합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector operator/(float Value) const;

    /**
     * @brief 대응 성분을 현재 값에 더합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FVector& operator+=(const FVector& Other);

    /**
     * @brief 대응 성분을 현재 값에서 뺍니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FVector& operator-=(const FVector& Other);

    /**
     * @brief 대응 성분끼리 곱한 결과를 현재 값에 반영합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FVector& operator*=(const FVector& Other);

    /**
     * @brief 나눗셈 결과를 현재 값에 반영합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FVector& operator/=(const FVector& Other);

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     * @param[in] Value 입력 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FVector& operator*=(float Value);

    /**
     * @brief 나눗셈 결과를 현재 값에 반영합니다.
     * @param[in] Value 입력 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FVector& operator/=(float Value);

    /**
     * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool operator==(const FVector& Other) const;

    /**
     * @brief 대응 성분의 곱을 합산하여 내적을 계산합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 대응 성분 곱의 합입니다.
     */
    float Dot(const FVector& Other) const;

    /**
     * @brief 두 벡터의 외적을 계산합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Cross(const FVector& Other) const;

    /**
     * @brief 벡터를 지정한 방향에 정사영합니다.
     * @param[in] Direction 투영 방향 또는 광선의 진행 방향입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector ProjectedOn(const FVector& Direction) const;

    /**
     * @brief 벡터를 지정한 법선의 평면에 정사영합니다.
     * @param[in] Normal 평면 또는 표면 법선입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector ProjectedOnPlane(const FVector& Normal) const;
    FVector Reflected(const FVector& Normal) const;

    /**
     * @brief 지정한 축을 중심으로 도 단위 각도만큼 회전합니다.
     * @param[in] Degrees 도 단위 각도입니다.
     * @param[in] Axis 회전 또는 편집에 사용할 축입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector RotatedAngleAxis(float Degrees, const FVector& Axis) const;

    /**
     * @brief 벡터 방향을 향하는 도 단위 Pitch와 Yaw 회전을 구합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator Rotation() const;

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
     * @note 유한하지 않거나 길이가 너무 작으면 영벡터로 초기화합니다.
     */
    FVector Normalized() const;

    /**
     * @brief 값을 제자리에서 정규화합니다.
     * @return 정규화할 수 있으면 true이며, 퇴화하거나 유효하지 않은 값이면 false입니다.
     * @note 유한하지 않거나 길이가 너무 작으면 영벡터로 초기화합니다.
     */
    bool Normalize();

    /**
     * @brief 두 값 사이 유클리드 거리의 제곱을 구합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 상대 값까지의 제곱 거리입니다.
     */
    float DistanceSquared(const FVector& Other) const;

    /**
     * @brief 두 값 사이 유클리드 거리를 구합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 상대 값까지의 유클리드 거리입니다.
     */
    float Distance(const FVector& Other) const;

    /**
     * @brief 두 값 사이를 주어진 비율로 선형 보간합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @param[in] Alpha 보간 비율입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Lerp(const FVector& Other, float Alpha) const;

    /**
     * @brief 각 성분의 절댓값을 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Abs() const;

    /**
     * @brief 각 성분에서 두 값 중 작은 값을 선택합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Min(const FVector& Other) const;

    /**
     * @brief 각 성분에서 두 값 중 큰 값을 선택합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Max(const FVector& Other) const;

    /**
     * @brief 각 성분을 지정한 최솟값과 최댓값 사이로 제한합니다.
     * @param[in] Minimum 성분별 하한입니다.
     * @param[in] Maximum 성분별 상한입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Clamped(const FVector& Minimum, const FVector& Maximum) const;

    /**
     * @brief 방향을 유지하며 벡터 길이를 지정한 상한으로 제한합니다.
     * @param[in] MaximumLength 허용할 최대 벡터 길이입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector ClampedLength(float MaximumLength) const;

    /**
     * @brief 모든 유효 성분이 허용 오차 이내에서 0인지 검사합니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsNearlyZero(float Tolerance = 0.0001f) const;

    /**
     * @brief 제곱 길이가 허용 오차 이내에서 1인지 검사합니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsNormalized(float Tolerance = 0.0001f) const;

    /**
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsFinite() const;

    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Equals(const FVector& Other, float Tolerance = 0.0001f) const;

    /**
     * @brief 유효한 성분을 원래 순서의 일반 배열로 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    std::array<float, 3> ToArray() const;

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
        };
    };
};

#pragma warning(pop)

static_assert(sizeof(FVector) == 16 && alignof(FVector) == 16);

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 * @param[in] Value 입력 값입니다.
 * @param[in] Vector 연산에 사용할 벡터입니다.
 * @return 연산을 적용한 새 값입니다.
 */
FVector operator*(float Value, const FVector& Vector);
