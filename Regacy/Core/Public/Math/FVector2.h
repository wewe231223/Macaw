/**
 * @file FVector2.h
 * @brief 2차원 단정밀도 벡터의 성분 접근과 산술 연산을 제공합니다.
 */
#pragma once
#include "Core/Types.h"
#include <cmath>

/**
 * @brief 2개의 float 성분을 갖는 벡터입니다.
 * @note V 배열과 이름 있는 성분은 같은 저장 공간을 공유합니다.
 */
struct FVector2 {
    /**
     * @brief 배열, 좌표 이름과 m 접두사 이름으로 같은 성분에 접근하는 저장 공간입니다.
     */
    union {
        /**
         * @brief X, Y 순서의 성분 배열입니다.
         */
        float V[2];

        /**
         * @brief X, Y 이름으로 벡터 성분에 접근하는 익명 구조체입니다.
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
        };
    };

    /**
     * @brief 모든 성분을 0으로 초기화합니다.
     */
    FVector2()
        : X(0),
          Y(0) {
    }

    /**
     * @brief 각 성분을 지정한 값으로 초기화합니다.
     * @param InX X 성분입니다.
     * @param InY Y 성분입니다.
     */
    FVector2(float InX, float InY)
        : X(InX),
          Y(InY) {
    }

    /**
     * @brief 모든 성분을 같은 값으로 초기화합니다.
     * @param InValue 모든 성분에 적용할 값입니다.
     */
    explicit FVector2(float InValue)
        : FVector2(InValue, InValue) {
    }

    /**
     * @brief 2개 성분의 제곱합을 반환합니다.
     * @return 벡터 길이의 제곱입니다.
     */
    float LengthSquared() const {
        return X * X + Y * Y;
    }

    /**
     * @brief 벡터의 유클리드 길이를 반환합니다.
     * @return X와 Y로 계산한 길이입니다.
     */
    float Length() const {
        return std::sqrt(LengthSquared());
    }

    /**
     * @brief 현재 벡터를 길이로 나누어 정규화합니다.
     * @note 길이가 0이면 성분을 유지합니다.
     */
    void Normalize() {
        const float VectorLength = Length();

        if (VectorLength > 0.0f) {
            X /= VectorLength;
            Y /= VectorLength;
        }
    }

    /**
     * @brief 인덱스에 해당하는 성분의 수정 가능한 참조를 반환합니다.
     * @param _index 0은 X, 1은 Y입니다.
     * @pre _index는 2 미만이어야 하며 범위를 검사하지 않습니다.
     * @return 선택한 성분의 참조입니다.
     */
    float& operator[](uint64 _index) {
        return V[_index];
    }

    /**
     * @brief 인덱스에 해당하는 성분의 읽기 전용 참조를 반환합니다.
     * @param _index 0은 X, 1은 Y입니다.
     * @pre _index는 2 미만이어야 하며 범위를 검사하지 않습니다.
     * @return 선택한 성분의 상수 참조입니다.
     */
    const float& operator[](uint64 _index) const {
        return V[_index];
    }

    /**
     * @brief 두 벡터의 성분별 뺄셈 결과를 반환합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 원본을 변경하지 않은 연산 결과입니다.
     */
    FVector2 operator-(const FVector2& Other) {
        return FVector2(X - Other.X, Y - Other.Y);
    }

    /**
     * @brief 두 벡터의 성분별 뺄셈 결과를 반환합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 원본을 변경하지 않은 연산 결과입니다.
     */
    FVector2 operator-(const FVector2& Other) const {
        return FVector2(X - Other.X, Y - Other.Y);
    }

    /**
     * @brief 두 벡터의 성분별 덧셈 결과를 반환합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 원본을 변경하지 않은 연산 결과입니다.
     */
    FVector2 operator+(const FVector2& Other) {
        return FVector2(X + Other.X, Y + Other.Y);
    }

    /**
     * @brief 두 벡터의 성분별 덧셈 결과를 반환합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 원본을 변경하지 않은 연산 결과입니다.
     */
    FVector2 operator+(const FVector2& Other) const {
        return FVector2(X + Other.X, Y + Other.Y);
    }

    /**
     * @brief 다른 벡터의 각 성분을 현재 성분에 더하고 결과를 저장합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 수정된 현재 벡터의 참조입니다.
     */
    FVector2& operator+=(const FVector2& Other) {
        X += Other.X;
        Y += Other.Y;

        return *this;
    }

    /**
     * @brief 각 성분에 스칼라를 곱한 새 벡터를 반환합니다.
     * @param Scalar 곱할 스칼라입니다.
     * @return 원본을 변경하지 않은 연산 결과입니다.
     */
    FVector2 operator*(const float Scalar) {
        return FVector2(X * Scalar, Y * Scalar);
    }

    /**
     * @brief 각 성분에 스칼라를 곱한 새 벡터를 반환합니다.
     * @param Scalar 곱할 스칼라입니다.
     * @return 원본을 변경하지 않은 연산 결과입니다.
     */
    FVector2 operator*(const float Scalar) const {
        return FVector2(X * Scalar, Y * Scalar);
    }

    /**
     * @brief 모든 성분을 스칼라로 나누어 현재 벡터에 저장합니다.
     * @param Scalar 나눌 스칼라입니다.
     * @pre Scalar는 0이 아니어야 합니다.
     * @return 수정된 현재 벡터의 참조입니다.
     */
    FVector2& operator/=(float Scalar) {
        X /= Scalar;
        Y /= Scalar;

        return *this;
    }
};

/**
 * @brief FVector2의 2차원 벡터 별칭입니다.
 */
using FVector2D = FVector2;
