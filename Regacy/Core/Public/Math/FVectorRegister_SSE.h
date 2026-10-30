/**
 * @file FVectorRegister_SSE.h
 * @brief x64 SSE 레지스터의 메모리 접근, 성분 연산, 내적과 재배치를 제공합니다.
 * @note Dot과 Dot4 및 이를 사용하는 연산은 SSE 4.1의 내적 명령을 사용합니다.
 */

#pragma once
#include <immintrin.h>

#if defined(__x86_64__) || defined(_M_X64)
// 인텔/AMD 환경
/**
 * @brief 네 개의 단정밀도 성분을 저장하는 x64 SSE 레지스터 타입입니다.
 */
using FVectorRegister = __m128;
#else
/**
 * @brief x64 매크로가 없는 환경의 16바이트 정렬 저장 구조체입니다.
 * @note 이 저장 선언과 별개로 아래 연산 함수는 SSE intrinsic을 사용합니다.
 */
struct alignas(16) FVectorRegister {
    /**
     * @brief 네 개의 단정밀도 성분을 저장하는 배열입니다.
     */
    float v[4];
};
#endif

/**
 * @brief X, Y, Z, W 순서의 네 float 성분에 대한 SSE 연산을 제공합니다.
 * @note 포인터 기반 함수는 유효한 저장 공간을 요구하며, Aligned 함수만 16바이트 정렬을 요구합니다.
 */
namespace VectorSIMD {
    /**
     * @brief 세 float를 X, Y, Z로 읽고 W를 0으로 설정합니다.
     * @param Ptr 읽을 연속된 세 float의 포인터입니다.
     * @return (Ptr[0], Ptr[1], Ptr[2], 0) 레지스터입니다.
     */
    FVectorRegister LoadFloat3(const float* Ptr);

    /**
     * @brief X, Y, Z를 연속된 세 float에 저장합니다.
     * @param[out] Ptr 세 float를 쓸 저장 공간입니다.
     * @param V 저장할 레지스터입니다. W는 저장하지 않습니다.
     */
    void StoreFloat3(float* Ptr, FVectorRegister V);

    /**
     * @brief 16바이트 정렬된 주소에서 네 float를 읽습니다.
     * @param Ptr 읽을 연속된 네 float의 포인터입니다.
     * @pre Ptr은 16바이트 경계에 정렬되어 있어야 합니다.
     * @return 읽은 네 성분입니다.
     */
    FVectorRegister LoadAligned(const float* Ptr);

    /**
     * @brief 네 성분을 16바이트 정렬된 주소에 저장합니다.
     * @param[out] Ptr 네 float를 쓸 저장 공간입니다.
     * @param V 저장할 레지스터입니다.
     * @pre Ptr은 16바이트 경계에 정렬되어 있어야 합니다.
     */
    void StoreAligned(float* Ptr, FVectorRegister V);

    /**
     * @brief X, Y, Z를 유지하고 W를 0으로 설정한 레지스터를 반환합니다.
     * @param A 입력 레지스터입니다.
     * @return W가 0인 결과입니다.
     */
    FVectorRegister SetWZero(FVectorRegister A);

    /**
     * @brief 각 성분의 제곱근 역수 근사값을 계산합니다.
     * @param V 입력 레지스터입니다.
     * @pre 유한한 실수 결과가 필요하면 각 성분은 유한한 양수이어야 합니다.
     * @return SSE rsqrt 명령으로 계산한 성분별 근사값입니다.
     */
    FVectorRegister Rsqrt(FVectorRegister V);

    /**
     * @brief 각 성분을 분모로 하는 1 / A를 계산합니다.
     * @param A 입력 레지스터입니다.
     * @pre 유한한 결과가 필요하면 각 성분은 0이 아니어야 합니다.
     * @return 성분별 역수입니다.
     */
    FVectorRegister Reciprocal(FVectorRegister A);

    /**
     * @brief 대응하는 성분의 최솟값을 선택합니다.
     * @param A 첫 번째 레지스터입니다.
     * @param B 두 번째 레지스터입니다.
     * @return SSE min 명령의 성분별 결과입니다.
     * @note NaN 입력이나 값이 같은 경우에는 B의 해당 성분을 선택합니다.
     */
    FVectorRegister Min(FVectorRegister A, FVectorRegister B);

    /**
     * @brief 대응하는 성분의 최댓값을 선택합니다.
     * @param A 첫 번째 레지스터입니다.
     * @param B 두 번째 레지스터입니다.
     * @return SSE max 명령의 성분별 결과입니다.
     * @note NaN 입력이나 값이 같은 경우에는 B의 해당 성분을 선택합니다.
     */
    FVectorRegister Max(FVectorRegister A, FVectorRegister B);

    /**
     * @brief 각 성분의 부호 비트를 제거합니다.
     * @param A 입력 레지스터입니다.
     * @return 성분별 절댓값 레지스터입니다.
     */
    FVectorRegister Abs(FVectorRegister A);

    /**
     * @brief 네 성분 모두의 절댓값이 허용 오차 이하인지 검사합니다.
     * @param A 검사할 레지스터입니다.
     * @param Tolerance 음수가 아닌 절대 허용 오차입니다.
     * @return 모든 성분이 조건을 만족하면 true입니다.
     */
    bool IsNearlyZero(FVectorRegister A, float Tolerance = 1e-4f);

    /**
     * @brief 네 성분 내적을 계산해 모든 결과 성분에 복제합니다.
     * @param A 첫 번째 벡터입니다.
     * @param B 두 번째 벡터입니다.
     * @return 네 성분이 모두 같은 내적 값인 레지스터입니다.
     */
    FVectorRegister Dot4(FVectorRegister A, FVectorRegister B);

    /**
     * @brief float 저장 비트에 성분별 비트 AND를 수행합니다.
     * @param A 첫 번째 레지스터입니다.
     * @param B 두 번째 레지스터입니다.
     * @return 비트 연산 결과 레지스터입니다.
     */
    FVectorRegister And(FVectorRegister A, FVectorRegister B);

    /**
     * @brief float 저장 비트에 성분별 비트 OR를 수행합니다.
     * @param A 첫 번째 레지스터입니다.
     * @param B 두 번째 레지스터입니다.
     * @return 비트 연산 결과 레지스터입니다.
     */
    FVectorRegister Or(FVectorRegister A, FVectorRegister B);

    /**
     * @brief float 저장 비트에 성분별 비트 XOR를 수행합니다.
     * @param A 첫 번째 레지스터입니다.
     * @param B 두 번째 레지스터입니다.
     * @return 비트 연산 결과 레지스터입니다.
     */
    FVectorRegister Xor(FVectorRegister A, FVectorRegister B);

    /**
     * @brief A의 비트를 반전한 뒤 B와 비트 AND를 수행합니다.
     * @param A 비트를 반전할 레지스터입니다.
     * @param B AND 연산의 오른쪽 레지스터입니다.
     * @return (~A) & B의 비트 패턴을 갖는 레지스터입니다.
     */
    FVectorRegister AndNot(FVectorRegister A, FVectorRegister B);

    /**
     * @brief 성분별 A * B + C를 계산합니다.
     * @param A 곱셈의 첫 번째 입력입니다.
     * @param B 곱셈의 두 번째 입력입니다.
     * @param C 더할 값입니다.
     * @return 곱셈 후 덧셈한 결과입니다.
     * @note 곱셈과 덧셈을 별도로 수행하며 단일 FMA 명령을 사용하지 않습니다.
     */
    FVectorRegister MultiplyAdd(FVectorRegister A, FVectorRegister B, FVectorRegister C);

    /**
     * @brief 성분별 C - A * B를 계산합니다.
     * @param A 곱셈의 첫 번째 입력입니다.
     * @param B 곱셈의 두 번째 입력입니다.
     * @param C 곱셈 결과를 뺄 기준값입니다.
     * @return 곱셈 후 뺄셈한 결과입니다.
     * @note 곱셈과 뺄셈을 별도로 수행합니다.
     */
    FVectorRegister NegativeMultiplyAdd(FVectorRegister A, FVectorRegister B, FVectorRegister C);

    /**
     * @brief 네 성분을 합산해 스칼라로 반환합니다.
     * @param V 합산할 레지스터입니다.
     * @return X + Y + Z + W에 해당하는 값입니다.
     */
    float HorizontalAdd4(FVectorRegister V);

    /**
     * @brief W를 포함한 네 성분으로 벡터를 정규화합니다.
     * @param A 정규화할 레지스터입니다.
     * @return 길이의 제곱이 유한한 양수이면 단위 길이 결과이고, 그 외에는 영벡터입니다.
     */
    FVectorRegister Normalized(FVectorRegister A);

    // SSE 구현
    /**
     * @brief 정렬 제한 없이 연속된 네 float를 읽습니다.
     * @param Ptr 읽을 네 float의 포인터입니다.
     * @return 읽은 네 성분 레지스터입니다.
     */
    inline FVectorRegister Load(const float* Ptr) {
        return _mm_loadu_ps(Ptr);
    }

    /**
     * @brief 정렬 제한 없이 네 성분을 연속된 네 float에 저장합니다.
     * @param[out] Ptr 네 float를 쓸 저장 공간입니다.
     * @param V 저장할 레지스터입니다.
     */
    inline void Store(float* Ptr, FVectorRegister V) {
        _mm_storeu_ps(Ptr, V);
    }

    /**
     * @brief 모든 성분이 0인 레지스터를 반환합니다.
     * @return (0, 0, 0, 0) 레지스터입니다.
     */
    inline FVectorRegister SetZero() {
        return _mm_setzero_ps();
    }

    /**
     * @brief 같은 값을 네 성분에 복제합니다.
     * @param F 복제할 값입니다.
     * @return (F, F, F, F) 레지스터입니다.
     */
    inline FVectorRegister SetVal(float F) {
        return _mm_set1_ps(F);
    }

    /**
     * @brief X, Y, Z, W 순서로 네 성분을 구성합니다.
     * @param X 첫 번째 성분입니다.
     * @param Y 두 번째 성분입니다.
     * @param Z 세 번째 성분입니다.
     * @param W 네 번째 성분입니다.
     * @return 지정한 네 성분 레지스터입니다.
     */
    inline FVectorRegister SetVal(float X, float Y, float Z, float W) {
        return _mm_setr_ps(X, Y, Z, W);
    }

    /**
     * @brief A의 각 성분에 B의 대응 성분을 더한 결과를 반환합니다.
     * @param A 왼쪽 피연산자입니다.
     * @param B 오른쪽 피연산자입니다.
     * @return 성분별 산술 연산 결과입니다.
     */
    inline FVectorRegister Add(FVectorRegister A, FVectorRegister B) {
        return _mm_add_ps(A, B);
    }

    /**
     * @brief A의 각 성분에 B의 대응 성분을 뺀 결과를 반환합니다.
     * @param A 왼쪽 피연산자입니다.
     * @param B 오른쪽 피연산자입니다.
     * @return 성분별 산술 연산 결과입니다.
     */
    inline FVectorRegister Sub(FVectorRegister A, FVectorRegister B) {
        return _mm_sub_ps(A, B);
    }

    /**
     * @brief A의 각 성분에 B의 대응 성분을 곱한 결과를 반환합니다.
     * @param A 왼쪽 피연산자입니다.
     * @param B 오른쪽 피연산자입니다.
     * @return 성분별 산술 연산 결과입니다.
     */
    inline FVectorRegister Mul(FVectorRegister A, FVectorRegister B) {
        return _mm_mul_ps(A, B);
    }

    /**
     * @brief A의 각 성분에 B의 대응 성분을 나눈 결과를 반환합니다.
     * @param A 왼쪽 피연산자입니다.
     * @param B 오른쪽 피연산자입니다.
     * @pre 유한한 결과가 필요하면 B의 각 성분은 0이 아니어야 합니다.
     * @return 성분별 산술 연산 결과입니다.
     */
    inline FVectorRegister Div(FVectorRegister A, FVectorRegister B) {
        return _mm_div_ps(A, B);
    }

    /**
     * @brief 네 성분을 허용 오차 없이 비교합니다.
     * @param _a 첫 번째 레지스터입니다.
     * @param _b 두 번째 레지스터입니다.
     * @return 대응하는 네 성분이 모두 같으면 true입니다.
     */
    inline bool Compare(FVectorRegister _a, FVectorRegister _b) {
        __m128 cmp = _mm_cmpeq_ps(_a, _b);

        return _mm_movemask_ps(cmp) == 0xF;
    }

    /**
     * @brief 모든 대응 성분의 차이가 절대 허용 오차 이내인지 검사합니다.
     * @param A 첫 번째 레지스터입니다.
     * @param B 두 번째 레지스터입니다.
     * @param Tolerance 음수가 아닌 절대 허용 오차입니다.
     * @return 모든 성분 차이의 절댓값이 허용 오차 이하이면 true입니다.
     */
    inline bool IsNearlyEqual(FVectorRegister A, FVectorRegister B, float Tolerance = 1e-4f) {
        // Step 1: 뺄셈 (A - B)
        FVectorRegister diff = _mm_sub_ps(A, B);

        // Step 2: abs
        // 0x7FFFFFFF (이진수로 01111111...) 마스크 생성
        // 이것과 AND 연산을 하면 맨 앞의 부호 비트가 무조건 0(양수)
        // _mm_castsi128_ps - 다시 float4 형태로 캐스팅
        FVectorRegister absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
        FVectorRegister absDiff = _mm_and_ps(diff, absMask);

        // Step 3: 엡실론 레지스터 준비 (4칸 모두 엡실론 값으로 꽉 채움)
        FVectorRegister epsReg = _mm_set1_ps(Tolerance);

        // Step 4: 작거나 같은가? (Compare Less-than or Equal)
        // absDiff <= epsReg 를 검사하여 맞으면 0xFFFFFFFF, 틀리면 0x0 을 반환
        FVectorRegister cmp = _mm_cmple_ps(absDiff, epsReg);

        // Step 5: 4개의 결과가 모두 0xFFFFFFFF(True) 인지 압축해서 확인
        return _mm_movemask_ps(cmp) == 0xF;
    }

    // 내적 (SSE 4.1 이상)
    /**
     * @brief W를 포함한 네 성분 내적을 스칼라로 반환합니다.
     * @param A 첫 번째 벡터입니다.
     * @param B 두 번째 벡터입니다.
     * @return 대응하는 네 성분 곱의 합입니다.
     */
    inline float Dot(FVectorRegister A, FVectorRegister B) {
        return _mm_cvtss_f32(_mm_dp_ps(A, B, 0xFF));
    }

    // 데이터 재배치 (외적 구현 시 필수)
    // 셔플은 매크로를 사용해야 하므로 템플릿으로 감싸는 것이 일반적
    /**
     * @brief 두 입력에서 성분을 골라 (A[X], A[Y], B[Z], B[W])를 구성합니다.
     * @tparam X 결과 첫 번째 성분에 사용할 A 인덱스입니다.
     * @tparam Y 결과 두 번째 성분에 사용할 A 인덱스입니다.
     * @tparam Z 결과 세 번째 성분에 사용할 B 인덱스입니다.
     * @tparam W 결과 네 번째 성분에 사용할 B 인덱스입니다.
     * @param A 결과의 앞 두 성분을 제공할 레지스터입니다.
     * @param B 결과의 뒤 두 성분을 제공할 레지스터입니다.
     * @pre 모든 템플릿 인덱스는 0 이상 4 미만이어야 합니다.
     * @return 재배치한 레지스터입니다.
     */
    template <int X, int Y, int Z, int W>
    inline FVectorRegister Shuffle(FVectorRegister A, FVectorRegister B) {
        return _mm_shuffle_ps(A, B, _MM_SHUFFLE(W, Z, Y, X));
        // 입력: Shuffle<0, 1, 2, 3>(A, B)  결과: [A0, A1, B2, B3]
    }

    /**
     * @brief 한 입력의 성분을 골라 (A[X], A[Y], A[Z], A[W])를 구성합니다.
     * @tparam X 결과 첫 번째 성분의 입력 인덱스입니다.
     * @tparam Y 결과 두 번째 성분의 입력 인덱스입니다.
     * @tparam Z 결과 세 번째 성분의 입력 인덱스입니다.
     * @tparam W 결과 네 번째 성분의 입력 인덱스입니다.
     * @param A 재배치할 레지스터입니다.
     * @pre 모든 템플릿 인덱스는 0 이상 4 미만이어야 합니다.
     * @return 재배치한 레지스터입니다.
     */
    template <int X, int Y, int Z, int W>
    inline FVectorRegister Swizzle(FVectorRegister A) {
        return _mm_shuffle_ps(A, A, _MM_SHUFFLE(W, Z, Y, X));
        // A = [10, 20, 30, 40], Swizzle<0, 0, 0, 0>(A),  [10, 10, 10, 10]
    }

    // X, Y, Z, W 중 하나를 4칸에 모두 복사 (행렬 곱셈 필수품)
    /**
     * @brief X, Y, Z 성분으로 A와 B의 외적을 계산합니다.
     * @param A 왼쪽 벡터입니다.
     * @param B 오른쪽 벡터입니다.
     * @return A × B이며 W를 0으로 설정한 레지스터입니다.
     */
    inline FVectorRegister Cross3(FVectorRegister A, FVectorRegister B) {
        FVectorRegister A_yzx = Shuffle<1, 2, 0, 3>(A, A);
        FVectorRegister B_zxy = Shuffle<2, 0, 1, 3>(B, B);

        FVectorRegister A_zxy = Shuffle<2, 0, 1, 3>(A, A);
        FVectorRegister B_yzx = Shuffle<1, 2, 0, 3>(B, B);

        FVectorRegister Result = Sub(Mul(A_yzx, B_zxy), Mul(A_zxy, B_yzx));

        return _mm_and_ps(Result, _mm_castsi128_ps(_mm_set_epi32(0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)));
    }

    /**
     * @brief 각 성분의 부호 비트를 반전합니다.
     * @param A 입력 레지스터입니다.
     * @return 성분별 부호를 반전한 레지스터입니다.
     */
    inline FVectorRegister Negate(FVectorRegister A) {
        FVectorRegister SignMask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));

        return _mm_xor_ps(A, SignMask);
    }

    /**
     * @brief W를 포함한 네 성분의 제곱합을 반환합니다.
     * @param A 길이를 계산할 레지스터입니다.
     * @return 4차원 벡터 길이의 제곱입니다.
     */
    inline float LengthSquared(FVectorRegister A) {
        return Dot(A, A);
    }

    // Matrix 관련
    /**
     * @brief 네 행 레지스터를 전치해 입력 레지스터에 덮어씁니다.
     * @param[in,out] A 입력의 첫 번째 행입니다. 전치 후 같은 인덱스의 행으로 교체합니다.
     * @param[in,out] B 입력의 두 번째 행입니다. 전치 후 같은 인덱스의 행으로 교체합니다.
     * @param[in,out] C 입력의 세 번째 행입니다. 전치 후 같은 인덱스의 행으로 교체합니다.
     * @param[in,out] D 입력의 네 번째 행입니다. 전치 후 같은 인덱스의 행으로 교체합니다.
     */
    inline void Transpose(FVectorRegister& A, FVectorRegister& B, FVectorRegister& C, FVectorRegister& D) {
        _MM_TRANSPOSE4_PS(A, B, C, D);
    }

}
