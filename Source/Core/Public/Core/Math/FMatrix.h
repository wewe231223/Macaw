/**
 * @file FMatrix.h
 * @brief 행 벡터 변환 규약을 사용하는 16바이트 정렬 4×4 행렬입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"

struct FTransform;
struct FVector4;

/**
 * @brief 행 벡터 변환 규약을 사용하는 16바이트 정렬 4×4 행렬입니다.
 * @ingroup Core
 */
struct alignas(16) FMatrix {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FMatrix);

private:
    /**
     * @brief SIMD 저장소를 사용하는 수학 연산 또는 객체 등록을 위해 내부 접근을 허용합니다.
     */
    friend struct FBox;

    /**
     * @brief SIMD 저장소를 사용하는 수학 연산 또는 객체 등록을 위해 내부 접근을 허용합니다.
     */
    friend struct FVector4;

public:
    /**
     * @brief FMatrix의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FMatrix();

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FMatrix() = default;

    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FMatrix(const FMatrix& Other) = default;

    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FMatrix& operator=(const FMatrix& Other) = default;

    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FMatrix(FMatrix&& Other) noexcept = default;

    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FMatrix& operator=(FMatrix&& Other) noexcept = default;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FMatrix);

    /**
     * @brief 항등행렬을 생성합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix Identity();

    /**
     * @brief 이동 성분을 갖는 변환 행렬을 생성합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix Translation(const FVector& Value);

    /**
     * @brief 축별 스케일 행렬을 생성합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix Scale(const FVector& Value);

    /**
     * @brief 회전 값에 대응하는 회전 행렬 또는 오일러 회전을 구합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix Rotation(const FRotator& Value);

    /**
     * @brief 회전 값에 대응하는 회전 행렬 또는 오일러 회전을 구합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix Rotation(const FQuat& Value);

    /**
     * @brief 지정한 축을 중심으로 라디안 단위 회전 행렬을 생성합니다.
     * @param[in] Axis 회전 또는 편집에 사용할 축입니다.
     * @param[in] Radians 라디안 단위 각도입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix RotationAxis(const FVector& Axis, float Radians);

    /**
     * @brief 위치, 회전과 스케일을 행 벡터 규약의 행렬로 합성합니다.
     * @param[in] Transform 적용할 위치, 회전과 스케일입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix FromTransform(const FTransform& Transform);

    /**
     * @brief 카메라 위치, 전방과 상방으로 뷰 행렬을 생성합니다.
     * @param[in] Position 변환하거나 투영할 위치입니다.
     * @param[in] Forward 카메라의 전방 방향입니다.
     * @param[in] Up 카메라의 상방 방향입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix LookTo(const FVector& Position, const FVector& Forward, const FVector& Up);

    /**
     * @brief 수직 시야각과 종횡비로 Direct3D 깊이 범위의 원근 투영 행렬을 생성합니다.
     * @param[in] FieldOfView 도 단위 수직 시야각입니다.
     * @param[in] Aspect 가로 크기를 세로 크기로 나눈 종횡비입니다.
     * @param[in] NearPlane 가까운 절단면까지의 거리입니다.
     * @param[in] FarPlane 먼 절단면까지의 거리입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    static FMatrix Perspective(float FieldOfView, float Aspect, float NearPlane, float FarPlane);

    /**
     * @brief 지정한 가로·세로 범위와 깊이 구간의 정사영 행렬을 만듭니다.
     * @param[in] Width 가로 길이 또는 픽셀 수입니다.
     * @param[in] Height 세로 길이 또는 픽셀 수입니다.
     * @param[in] NearPlane 가까운 클리핑 평면 거리입니다.
     * @param[in] FarPlane 먼 클리핑 평면 거리입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    static FMatrix Orthographic(float Width, float Height, float NearPlane, float FarPlane);

    /**
     * @brief 대응 성분을 더한 값을 반환합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FMatrix operator+(const FMatrix& Other) const;

    /**
     * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FMatrix operator-(const FMatrix& Other) const;

    /**
     * @brief 현재 행렬 뒤에 상대 행렬을 적용하는 행 벡터 규약의 곱을 계산합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FMatrix operator*(const FMatrix& Other) const;

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FMatrix operator*(float Value) const;

    /**
     * @brief 성분별 또는 스칼라 나눗셈 결과를 반환합니다.
     * @param[in] Value 입력 값입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FMatrix operator/(float Value) const;

    /**
     * @brief 대응 성분을 현재 값에 더합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FMatrix& operator+=(const FMatrix& Other);

    /**
     * @brief 대응 성분을 현재 값에서 뺍니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FMatrix& operator-=(const FMatrix& Other);

    /**
     * @brief 현재 행렬 뒤에 상대 행렬을 적용하는 행 벡터 규약의 곱을 계산합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FMatrix& operator*=(const FMatrix& Other);

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     * @param[in] Value 입력 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FMatrix& operator*=(float Value);

    /**
     * @brief 나눗셈 결과를 현재 값에 반영합니다.
     * @param[in] Value 입력 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FMatrix& operator/=(float Value);

    /**
     * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool operator==(const FMatrix& Other) const;

    /**
     * @brief 동차 W=1인 위치를 변환하고 결과 W로 나눕니다.
     * @param[in] Position 변환하거나 투영할 위치입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector TransformPosition(const FVector& Position) const;

    /**
     * @brief 방향 벡터에 스케일과 회전을 적용합니다.
     * @param[in] Vector 연산에 사용할 벡터입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector TransformVector(const FVector& Vector) const;

    /**
     * @brief 행렬에서 지정한 행의 XYZ 축 성분을 반환합니다.
     * @param[in] Index 선택할 요소 또는 축의 0부터 시작하는 인덱스입니다.
     * @return 행렬에서 지정한 행의 XYZ 축 성분입니다.
     * @note Index는 0부터 3까지여야 합니다.
     */
    FVector GetAxis(Int32 Index) const;

    /**
     * @brief 회전 값을 반환합니다.
     * @return 회전 값입니다.
     */
    FRotator GetRotation() const;

    /**
     * @brief 행렬에서 추출한 쿼터니언 회전을 반환합니다.
     * @return 행렬에서 추출한 쿼터니언 회전입니다.
     */
    FQuat GetQuaternion() const;

    /**
     * @brief 역행렬의 유효성을 검사하고 계산 가능한 경우 결과에 기록합니다.
     * @param[in,out] Result 변환 또는 파싱이 성공한 경우 결과를 기록합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool TryInverse(FMatrix& Result) const;

    /**
     * @brief 행과 열을 교환한 전치행렬을 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FMatrix Transposed() const;

    /**
     * @brief 4×4 행렬의 행렬식을 구합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    float Determinant() const;

    /**
     * @brief 행렬의 이동 성분을 반환합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Origin() const;

    /**
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsFinite() const;

    /**
     * @brief 현재 변환이 허용 오차 이내에서 항등 변환인지 검사합니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsIdentity(float Tolerance = 0.0001f) const;

    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Equals(const FMatrix& Other, float Tolerance = 0.0001f) const;

    /**
     * @brief 지정한 행과 열의 행렬 성분을 참조합니다.
     * @param[in] Row 0부터 시작하는 행 인덱스입니다.
     * @param[in] Column 0부터 시작하는 열 인덱스입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     * @note Row와 Column은 각각 0부터 3까지여야 합니다.
     */
    float& operator()(Int32 Row, Int32 Column);

    /**
     * @brief 지정한 행과 열의 행렬 성분을 참조합니다.
     * @param[in] Row 0부터 시작하는 행 인덱스입니다.
     * @param[in] Column 0부터 시작하는 열 인덱스입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     * @note Row와 Column은 각각 0부터 3까지여야 합니다.
     */
    const float& operator()(Int32 Row, Int32 Column) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief SIMD 행 벡터 네 개입니다.
     */
    __m128 mRows[4]{};
    ReflectProperty(mRows, "Rows", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 * @param[in] Value 입력 값입니다.
 * @param[in] Matrix 적용할 변환 행렬입니다.
 * @return 연산을 적용한 새 값입니다.
 */
FMatrix operator*(float Value, const FMatrix& Matrix);
