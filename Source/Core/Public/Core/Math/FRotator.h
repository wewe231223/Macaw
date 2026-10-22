/**
 * @file FRotator.h
 * @brief 도 단위의 Pitch, Yaw, Roll로 표현하는 오일러 회전입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <xmmintrin.h>

struct FQuat;
struct FVector;

#pragma warning(push)
#pragma warning(disable: 4201)

/**
 * @brief 도 단위의 Pitch, Yaw, Roll로 표현하는 오일러 회전입니다.
 * @ingroup Core
 */
struct alignas(16) FRotator {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FRotator);

public:
    /**
     * @brief FRotator의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FRotator() = default;
    /**
     * @brief FRotator의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Pitch Y축 주위 회전각(도)입니다.
     * @param[in] Yaw Z축 주위 회전각(도)입니다.
     * @param[in] Roll X축 주위 회전각(도)입니다.
     */
    FRotator(float Pitch, float Yaw, float Roll);
    /**
     * @brief FRotator의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Rotation 적용할 오일러 또는 쿼터니언 회전입니다.
     */
    explicit FRotator(const FQuat& Rotation);

private:
    /**
     * @brief FRotator의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Value 연산할 SIMD 레지스터 값입니다.
     */
    explicit FRotator(__m128 Value);

public:
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FRotator() = default;
    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FRotator(const FRotator& Other) = default;
    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FRotator& operator=(const FRotator& Other) = default;
    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FRotator(FRotator&& Other) noexcept = default;
    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FRotator& operator=(FRotator&& Other) noexcept = default;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FRotator);

    /**
     * @brief 대응 성분을 더한 값을 반환합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator operator+(const FRotator& Other) const;
    /**
     * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator operator-(const FRotator& Other) const;
    /**
     * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator operator-() const;
    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator operator*(float Value) const;
    /**
     * @brief 성분별 또는 스칼라 나눗셈 결과를 반환합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator operator/(float Value) const;
    /**
     * @brief 대응 성분을 현재 값에 더합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FRotator& operator+=(const FRotator& Other);
    /**
     * @brief 대응 성분을 현재 값에서 뺍니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FRotator& operator-=(const FRotator& Other);
    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     * @param[in] Value 입력 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FRotator& operator*=(float Value);
    /**
     * @brief 나눗셈 결과를 현재 값에 반영합니다.
     * @param[in] Value 입력 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FRotator& operator/=(float Value);
    /**
     * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool operator==(const FRotator& Other) const;

    /**
     * @brief 도 단위 오일러 각을 쿼터니언 회전으로 변환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FQuat Quaternion() const;
    /**
     * @brief 오일러 회전의 전방 단위 벡터를 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Vector() const;
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
     * @brief 현재 회전의 역회전을 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator Inverse() const;
    /**
     * @brief 각 오일러 각도를 360도 나머지 범위로 정규화한 복사본을 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator Normalized() const;
    /**
     * @brief 각 오일러 각도를 360도 나머지 범위로 정규화합니다.
     */
    void Normalize();
    /**
     * @brief 상대 회전과의 오일러 각 차이를 정규화하여 반환합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator Delta(const FRotator& Other) const;
    /**
     * @brief 두 값 사이를 주어진 비율로 선형 보간합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @param[in] Alpha 보간 비율입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FRotator Lerp(const FRotator& Other, float Alpha) const;
    /**
     * @brief 모든 유효 성분이 허용 오차 이내에서 0인지 검사합니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsNearlyZero(float Tolerance = 0.0001f) const;
    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Equals(const FRotator& Other, float Tolerance = 0.0001f) const;
    /**
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsFinite() const;

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
             * @brief Y축 주위 Pitch 각도(도)입니다.
             */
            float pitch;
            ReflectProperty(pitch, "Pitch", EPropertyFlags::Edit | EPropertyFlags::Serialize);


            /**
             * @brief Z축 주위 Yaw 각도(도)입니다.
             */
            float yaw;
            ReflectProperty(yaw, "Yaw", EPropertyFlags::Edit | EPropertyFlags::Serialize);


            /**
             * @brief X축 주위 Roll 각도(도)입니다.
             */
            float roll;
            ReflectProperty(roll, "Roll", EPropertyFlags::Edit | EPropertyFlags::Serialize);
        };
    };
};

#pragma warning(pop)

static_assert(sizeof(FRotator) == 16 && alignof(FRotator) == 16);

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 * @param[in] Value 입력 값입니다.
 * @param[in] Rotation 적용할 오일러 또는 쿼터니언 회전입니다.
 * @return 연산을 적용한 새 값입니다.
 */
FRotator operator*(float Value, const FRotator& Rotation);
