/**
 * @file FVector4.h
 * @brief 4차원 단정밀도 벡터의 성분 접근과 산술 연산을 제공합니다.
 */

#pragma once
#include "Core/Types.h"

//template<typename T>
//struct TVector;
struct FVector;
struct FMatrix;

/**
 * @brief 4개의 float 성분을 갖는 벡터입니다.
 * @note V 배열과 이름 있는 성분은 같은 저장 공간을 공유합니다.
 */
struct FVector4 {
    /**
     * @brief 배열, 좌표 이름과 m 접두사 이름으로 같은 성분에 접근하는 저장 공간입니다.
     */
    union {
        /**
         * @brief X, Y, Z, W 순서의 성분 배열입니다.
         */
        float V[4];

        /**
         * @brief X, Y, Z, W 이름으로 벡터 성분에 접근하는 익명 구조체입니다.
         */
        struct
        {
            /**
             * @brief X 성분입니다.
             */
            float X;

            /**
             * @brief Y 성분입니다.
             */
            float Y;

            /**
             * @brief Z 성분입니다.
             */
            float Z;

            /**
             * @brief W 성분입니다.
             */
            float W;
        };

        /**
         * @brief m 접두사 이름으로 같은 벡터 성분에 접근하는 익명 구조체입니다.
         */
        struct
        {
            /**
             * @brief X와 저장 공간을 공유하는 성분 별칭입니다.
             */
            float mX;

            /**
             * @brief Y와 저장 공간을 공유하는 성분 별칭입니다.
             */
            float mY;

            /**
             * @brief Z와 저장 공간을 공유하는 성분 별칭입니다.
             */
            float mZ;

            /**
             * @brief W와 저장 공간을 공유하는 성분 별칭입니다.
             */
            float mW;
        };
    };

    /* Constructor */
    /**
     * @brief 모든 성분을 0으로 초기화합니다.
     */
    FVector4();

    /**
     * @brief 각 성분을 지정한 값으로 초기화합니다.
     * @param x X 성분입니다.
     * @param y Y 성분입니다.
     * @param z Z 성분입니다.
     * @param w W 성분입니다.
     */
    FVector4(float x, float y, float z, float w);

    /**
     * @brief 모든 성분을 같은 값으로 초기화합니다.
     * @param f 모든 성분에 적용할 값입니다.
     */
    explicit FVector4(float f);

    /**
     * @brief 3차원 벡터에 W 성분을 추가해 초기화합니다.
     * @param V1 복사할 X, Y, Z 값입니다.
     * @param w W 성분입니다. 기본값 0은 방향 벡터를 표현할 때 사용합니다.
     */
    FVector4(const FVector& V1, float w = 0.0f);
    // ~FVector();

public:
    /* Public Functions */
    /**
     * @brief 모든 성분을 지정한 값으로 설정합니다.
     * @param x X 성분입니다.
     * @param y Y 성분입니다.
     * @param z Z 성분입니다.
     * @param w W 성분입니다.
     */
    void Set(float x, float y, float z, float w);

    /**
     * @brief 3차원 벡터와 W 값으로 성분을 설정합니다.
     * @param V1 복사할 X, Y, Z 값입니다.
     * @param w W 성분입니다.
     */
    void Set(const FVector& V1, float w = 0.0f);

    /**
     * @brief 다른 벡터의 모든 성분을 복사합니다.
     * @param V1 복사할 벡터입니다.
     */
    void Set(const FVector4& V1);

    /**
     * @brief 벡터의 유클리드 길이를 반환합니다.
     * @return 4개 성분으로 계산한 길이입니다.
     */
    float Size() const;   // 길이 반환

    /**
     * @brief 벡터의 유클리드 길이를 반환합니다.
     * @return Size와 같은 길이입니다.
     */
    float Length() const; // ==  size()

    /**
     * @brief 4개 성분의 제곱합을 반환합니다.
     * @return 벡터 길이의 제곱입니다.
     */
    float LengthSquared() const;

    /**
     * @brief 다른 벡터와 4차원 내적을 계산합니다.
     * @param V1 내적할 벡터입니다.
     * @return 대응하는 성분 곱의 합입니다.
     */
    float Dot(const FVector4& V1) const;

    /**
     * @brief 현재 벡터를 길이로 나누어 정규화합니다.
     * @note 길이가 1e-8 이하이면 성분을 유지합니다.
     */
    void Normalize();

    /**
     * @brief X, Y, Z 성분으로 외적을 계산합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @return this와 V1의 순서로 계산한 외적이며 W는 0입니다.
     */
    FVector4 Cross(const FVector4& V1) const;

    /**
     * @brief 각 성분의 절댓값을 갖는 벡터를 반환합니다.
     * @return 성분별 절댓값 벡터입니다.
     */
    FVector4 GetAbs() const;

    // float GetMax();
    //// float GetMin();
    // float GetAbsMax();
    // float GetAbsMin();

    // GetSafeNormal

    // ClampSize

    /* operator */

    /**
     * @brief 모든 성분의 부호를 반전한 새 벡터를 반환합니다.
     * @return 원본을 변경하지 않은 부호 반전 결과입니다.
     */
    FVector4 operator-();

    /**
     * @brief 두 벡터의 성분별 뺄셈 결과를 반환합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @return 원본을 변경하지 않은 연산 결과입니다.
     */
    FVector4 operator-(const FVector4& V1) const;

    /**
     * @brief 다른 벡터의 각 성분을 현재 성분에 빼고 결과를 저장합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @return 수정된 현재 벡터의 참조입니다.
     */
    FVector4& operator-=(const FVector4& V1);

    /**
     * @brief 두 벡터의 성분별 덧셈 결과를 반환합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @return 원본을 변경하지 않은 연산 결과입니다.
     */
    FVector4 operator+(const FVector4& V1) const;

    /**
     * @brief 다른 벡터의 각 성분을 현재 성분에 더하고 결과를 저장합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @return 수정된 현재 벡터의 참조입니다.
     */
    FVector4& operator+=(const FVector4& V1);

    /**
     * @brief 두 벡터의 성분별 곱을 반환합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @return 성분별 연산 결과입니다.
     */
    FVector4 operator*(const FVector4& V1) const;

    /**
     * @brief 각 성분을 스칼라로 곱한 벡터를 반환합니다.
     * @param f 곱할 스칼라입니다.
     * @return 연산 결과 벡터입니다.
     */
    FVector4 operator*(const float& f) const;

    /**
     * @brief 현재 벡터를 행 벡터로 취급해 행렬로 변환합니다.
     * @param M 적용할 행렬입니다.
     * @return W 나눗셈을 수행하지 않은 4차원 변환 결과입니다.
     */
    FVector4 operator*(const FMatrix& M) const;

    /**
     * @brief 다른 벡터의 각 성분을 현재 성분에 곱하고 결과를 저장합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @return 수정된 현재 벡터의 참조입니다.
     */
    FVector4& operator*=(const FVector4& V1);

    /**
     * @brief 모든 성분을 스칼라로 곱해 현재 벡터에 저장합니다.
     * @param f 곱할 스칼라입니다.
     * @return 수정된 현재 벡터의 참조입니다.
     */
    FVector4& operator*=(const float& f);

    /**
     * @brief 두 벡터의 성분별 몫을 반환합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @pre V1의 모든 성분은 0이 아니어야 합니다.
     * @return 성분별 연산 결과입니다.
     */
    FVector4 operator/(const FVector4& V1) const;

    /**
     * @brief 각 성분을 스칼라로 나눈 벡터를 반환합니다.
     * @param f 나눌 스칼라입니다.
     * @pre f는 0이 아니어야 합니다.
     * @return 연산 결과 벡터입니다.
     */
    FVector4 operator/(const float& f) const;

    /**
     * @brief 다른 벡터의 각 성분을 현재 성분에 나누고 결과를 저장합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @pre 모든 분모 성분은 0이 아니어야 합니다.
     * @return 수정된 현재 벡터의 참조입니다.
     */
    FVector4& operator/=(const FVector4& V1);

    /**
     * @brief 모든 성분을 스칼라로 나누어 현재 벡터에 저장합니다.
     * @param f 나눌 스칼라입니다.
     * @pre f는 0이 아니어야 합니다.
     * @return 수정된 현재 벡터의 참조입니다.
     */
    FVector4& operator/=(const float& f);

    /**
     * @brief Cross와 같은 순서로 외적을 계산합니다.
     * @param V1 오른쪽 피연산자입니다.
     * @return 외적 결과이며 W는 0입니다.
     */
    FVector4 operator^(const FVector4& V1) const;

    /**
     * @brief 성분을 허용 오차 없이 비교합니다.
     * @param V1 비교할 벡터입니다.
     * @return 모든 성분이 같으면 true입니다.
     */
    bool operator==(const FVector4& V1) const;

    /**
     * @brief 성분을 허용 오차 없이 비교합니다.
     * @param V1 비교할 벡터입니다.
     * @return 하나 이상의 성분이 다르면 true입니다.
     */
    bool operator!=(const FVector4& V1) const;

    /**
     * @brief 인덱스에 해당하는 성분 값을 반환합니다.
     * @param Index X, Y, Z, W 순서의 0부터 시작하는 인덱스입니다.
     * @pre Index는 0 이상 4 미만이어야 하며 범위를 검사하지 않습니다.
     * @return 선택한 성분 값입니다.
     */
    float operator[](int32 Index) const;

    /**
     * @brief 인덱스에 해당하는 성분의 수정 가능한 참조를 반환합니다.
     * @param Index X, Y, Z, W 순서의 0부터 시작하는 인덱스입니다.
     * @pre Index는 0 이상 4 미만이어야 하며 범위를 검사하지 않습니다.
     * @return 선택한 성분의 참조입니다.
     */
    float& operator[](int32 Index);

    /**
     * @brief 두 벡터 사이의 4차원 유클리드 거리를 계산합니다.
     * @param V1 첫 번째 벡터입니다.
     * @param V2 두 번째 벡터입니다.
     * @return 두 벡터의 차이의 길이입니다.
     */
    static float Distance(const FVector4& V1, const FVector4& V2); // == Dist()
};

/**
 * @brief 네 성분 색상 저장에 사용하는 FVector4 별칭입니다.
 * @note 색상 범위 제한이나 색 공간 변환은 수행하지 않습니다.
 */
using FColor4 = FVector4;

/* Global Operator */
/**
 * @brief 벡터 성분을 괄호와 쉼표로 구분해 스트림에 출력합니다.
 * @param OS 출력 스트림입니다.
 * @param V 출력할 벡터입니다.
 * @return OS의 참조입니다.
 */
std::ostream& operator<<(std::ostream& OS, const FVector4& V);
