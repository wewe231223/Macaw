/**
 * @file FMatrixRegister.h
 * @brief 네 개의 SIMD 행 레지스터로 행렬의 전치, 행렬식과 역행렬 연산을 수행합니다.
 */
#pragma once
#include "Math/FMatrix.h"
#include "Math/FVectorRegister.h"

/**
 * @brief 4×4 행렬을 네 개의 FVectorRegister 행으로 저장하는 SIMD 표현입니다.
 * @note 기본 생성 값은 영행렬입니다.
 */
struct FMatrixRegister {
    /**
     * @brief 각 레지스터가 한 행의 네 성분을 저장하는 배열입니다.
     */
    FVectorRegister R[4];

public:
    /**
     * @brief 네 행 레지스터의 모든 성분을 0으로 초기화합니다.
     */
    FMatrixRegister();
    /**
     * @brief 전달한 네 레지스터로 행렬을 초기화합니다.
     * @param R0 첫 번째 행입니다.
     * @param R1 두 번째 행입니다.
     * @param R2 세 번째 행입니다.
     * @param R3 네 번째 행입니다.
     */
    FMatrixRegister(FVectorRegister R0, FVectorRegister R1, FVectorRegister R2, FVectorRegister R3);

    /**
     * @brief 네 행을 FMatrix 저장 공간에 복사합니다.
     * @return 같은 성분을 갖는 FMatrix입니다.
     */
    FMatrix ToFMatrix() const;
    /**
     * @brief 네 행 레지스터를 전치한 새 행렬을 반환합니다.
     * @return 행과 열을 교환한 결과입니다.
     */
    FMatrixRegister Transpose() const;

    /**
     * @brief 세 레지스터의 X, Y, Z를 행으로 하는 3×3 행렬식을 계산합니다.
     * @param A 첫 번째 행입니다.
     * @param B 두 번째 행입니다.
     * @param C 세 번째 행입니다.
     * @return A와 B × C의 내적입니다. W 성분은 제외합니다.
     */
    float Determinant3x3(FVectorRegister A, FVectorRegister B, FVectorRegister C) const;

    // 행렬식
    /**
     * @brief 현재 4×4 행렬의 행렬식을 계산합니다.
     * @return 행렬식입니다.
     */
    float Determinant() const;
    // 역행렬
    /**
     * @brief 여인수와 행렬식으로 역행렬을 계산합니다.
     * @return 역행렬입니다. 행렬식의 절댓값이 1e-8 이하이면 단위 행렬입니다.
     */
    FMatrixRegister Inverse() const;

    /**
     * @brief FMatrix의 네 행을 SIMD 레지스터로 읽습니다.
     * @param M 읽을 행렬입니다. 16바이트 정렬을 요구하지 않습니다.
     * @return 같은 행렬 성분을 갖는 SIMD 표현입니다.
     */
    static FMatrixRegister Load(const FMatrix& M);

    /**
     * @brief 단위 행렬을 구성하는 네 SIMD 행을 반환합니다.
     * @return 대각 성분이 1이고 나머지 성분이 0인 행렬입니다.
     */
    inline static FMatrixRegister Identity();
};
