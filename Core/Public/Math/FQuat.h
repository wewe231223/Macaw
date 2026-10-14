/**
 * @file FQuat.h
 * @brief 회전 쿼터니언의 합성, 역원, 벡터 회전과 각도·행렬 변환을 제공합니다.
 */
#pragma once
#include "Math/FVectorRegister.h"
#include "Math/SimpleMath/SimpleMath.h"

/**
 * @brief X, Y, Z, W 순서의 쿼터니언 SIMD 저장에 사용하는 벡터 레지스터 별칭입니다.
 */
using FQuatRegister = FVectorRegister;

//template<typename T>
//struct TVector;
struct FVector;
struct FVector4;
struct FMatrix;
struct FRotator;

/**
 * @brief 벡터부 X, Y, Z와 스칼라부 W로 회전을 표현하는 쿼터니언입니다.
 * @note 회전 연산에는 정규화된 쿼터니언을 사용합니다. 엔진의 Forward, Right, Up 기준 축은 각각 X, Y, Z입니다.
 */
struct FQuat {
    /**
     * @brief 배열과 이름 있는 성분이 같은 저장 공간을 공유하는 쿼터니언 저장 공간입니다.
     */
    union {
        /**
         * @brief X, Y, Z, W 순서의 성분 배열입니다.
         */
        float V[4];

        struct {
            /**
             * @brief 벡터부 X 성분입니다.
             */
            float X;
            /**
             * @brief 벡터부 Y 성분입니다.
             */
            float Y;
            /**
             * @brief 벡터부 Z 성분입니다.
             */
            float Z;
            /**
             * @brief 스칼라부 W 성분입니다.
             */
            float W;
        };

        struct {
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

    /**
     * @brief 회전이 없는 단위 쿼터니언 (0, 0, 0, 1)로 초기화합니다.
     */
    FQuat();
    /**
     * @brief 지정한 네 성분을 그대로 저장합니다.
     * @param InX 벡터부 X입니다.
     * @param InY 벡터부 Y입니다.
     * @param InZ 벡터부 Z입니다.
     * @param InW 스칼라부 W입니다.
     * @note 자동으로 정규화하지 않습니다.
     */
    FQuat(float InX, float InY, float InZ, float InW = 1.0f);
    /**
     * @brief 도 단위 회전 각도로 정규화된 쿼터니언을 생성합니다.
     * @param InRotator 변환할 Pitch, Yaw, Roll입니다.
     */
    explicit FQuat(const FRotator& InRotator);
    /**
     * @brief 3차원 벡터를 벡터부로, InW를 스칼라부로 복사합니다.
     * @param InVector X, Y, Z 성분입니다.
     * @param InW W 성분입니다.
     */
    FQuat(const FVector& InVector, float InW);
    /**
     * @brief 4차원 벡터의 X, Y, Z, W를 복사합니다.
     * @param InVector 복사할 네 성분입니다.
     */
    FQuat(const FVector4& InVector);
    /**
     * @brief DirectX의 네 성분을 복사합니다.
     * @param Value 복사할 x, y, z, w 값입니다.
     */
    explicit FQuat(const DirectX::XMFLOAT4& Value);
    /**
     * @brief 성분을 DirectX SimpleMath 쿼터니언으로 복사합니다.
     * @return 같은 X, Y, Z, W 값을 갖는 쿼터니언입니다.
     */
    DirectX::SimpleMath::Quaternion ToSimpleMath() const;

    /* Function */
    /**
     * @brief 현재 쿼터니언과 Other의 해밀턴 곱을 계산합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return this와 Other의 곱입니다. 단위 회전에서는 Other의 회전이 먼저 적용됩니다.
     * @note 결과를 자동으로 정규화하지 않습니다.
     */
    FQuat operator*(const FQuat& Other) const;
    /**
     * @brief 벡터부의 부호를 반전한 켤레 쿼터니언을 반환합니다.
     * @return (-X, -Y, -Z, W) 값입니다.
     */
    FQuat Conjugate() const;
    /**
     * @brief 켤레를 길이의 제곱으로 나눈 역원을 계산합니다.
     * @pre 길이의 제곱이 0이 아니어야 합니다.
     * @return 곱셈 역원입니다. 단위 쿼터니언에서는 켤레와 같습니다.
     */
    FQuat Inverse() const;
    /**
     * @brief 현재 쿼터니언을 단위 길이로 정규화합니다.
     * @note 길이의 제곱이 1e-8 이하이면 (0, 0, 0, 1)로 설정합니다.
     */
    void Normalize();
    /**
     * @brief 현재 쿼터니언으로 3차원 벡터를 회전합니다.
     * @param Vector 회전할 벡터입니다.
     * @pre 현재 쿼터니언은 정규화되어 있어야 합니다.
     * @return 회전한 벡터입니다.
     */
    FVector RotateVector(const FVector& Vector) const;

    /**
     * @brief 도 단위 회전 각도를 정규화된 쿼터니언으로 변환합니다.
     * @param Rotation Pitch는 Y축, Yaw는 Z축, Roll은 X축 회전입니다.
     * @return (Yaw * Pitch) * Roll 순서로 결합하고 정규화한 쿼터니언입니다.
     */
    static FQuat FromRotator(const FRotator& Rotation);
    /**
     * @brief First와 Second의 해밀턴 곱을 반환합니다.
     * @param First 왼쪽 피연산자입니다.
     * @param Second 오른쪽 피연산자입니다.
     * @return First * Second입니다. 단위 회전에서는 Second를 적용한 뒤 First를 적용합니다.
     */
    static FQuat Concatenate(const FQuat& First, const FQuat& Second);
    /**
     * @brief 쿼터니언을 도 단위 Pitch, Yaw, Roll로 변환합니다.
     * @pre 현재 쿼터니언은 정규화되어 있어야 합니다.
     * @return Y축 Pitch, Z축 Yaw, X축 Roll 각도입니다.
     */
    FRotator ToRotator() const;
    /**
     * @brief 쿼터니언으로 회전한 X축 방향을 반환합니다.
     * @pre 현재 쿼터니언은 정규화되어 있어야 합니다.
     * @return Forward 방향입니다.
     */
    FVector GetForwardVector() const;
    /**
     * @brief 쿼터니언으로 회전한 Y축 방향을 반환합니다.
     * @pre 현재 쿼터니언은 정규화되어 있어야 합니다.
     * @return Right 방향입니다.
     */
    FVector GetRightVector() const;
    /**
     * @brief 쿼터니언으로 회전한 Z축 방향을 반환합니다.
     * @pre 현재 쿼터니언은 정규화되어 있어야 합니다.
     * @return Up 방향입니다.
     */
    FVector GetUpVector() const;

    /* Static */
    /**
     * @brief 행 벡터 회전 행렬의 3×3 부분에서 쿼터니언을 추출합니다.
     * @param Matrix 회전 행렬입니다.
     * @pre 회전 부분에서 스케일과 전단이 제거되어 있어야 합니다.
     * @return 추출한 회전 쿼터니언입니다.
     */
    static const FQuat CreateFromRotationMatrix(const FMatrix& Matrix);
    /**
     * @brief 회전축과 라디안 각도로 쿼터니언을 생성합니다.
     * @param Axis 내부에서 정규화할 회전축입니다.
     * @param AngleRadians 라디안 단위 회전 각도입니다.
     * @pre Axis의 길이는 1e-8보다 커야 합니다.
     * @return 축의 정규화 성분에 sin(AngleRadians / 2)를 곱하고 W를 cos(AngleRadians / 2)로 설정한 쿼터니언입니다.
     */
    static const FQuat CreateFromAxisAngle(const FVector Axis, const float AngleRadians);
};
