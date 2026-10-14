/**
 * @file FMatrix.h
 * @brief 행 벡터 규약의 4×4 변환 행렬과 투영·역행렬·분해 연산을 제공합니다.
 */
#pragma once
#include "Core/Types.h"
#include "Math/SimpleMath/SimpleMath.h"
#include <cmath>
#include <limits>

struct FVector;
struct FVector4;
struct FRotator;
struct FQuat;

/**
 * @brief 행 벡터에 오른쪽에서 곱하는 4×4 단정밀도 행렬입니다.
 * @note 이동 성분은 M[3][0..2]에 저장합니다. 기본 생성 값은 단위 행렬이며 기준 축은 Forward=X, Right=Y, Up=Z입니다.
 */
struct FMatrix {
    /**
     * @brief M과 m_ 이름으로 같은 행렬 저장 공간에 접근합니다.
     */
    union {
        /**
         * @brief M[행][열]로 접근하는 행 우선 저장 배열입니다.
         */
        float M[4][4];
        /**
         * @brief M과 저장 공간을 공유하는 행렬 배열 별칭입니다.
         */
        float m_[4][4];
    };

public:
    /**
     * @brief 대각 성분이 1이고 나머지 성분이 0인 단위 행렬로 초기화합니다.
     */
    FMatrix();
    /**
     * @brief 네 벡터를 행렬의 각 행에 복사합니다.
     * @param InX 첫 번째 행입니다.
     * @param InY 두 번째 행입니다.
     * @param InZ 세 번째 행입니다.
     * @param InW 네 번째 행입니다.
     */
    FMatrix(const FVector4& InX, const FVector4& InY, const FVector4& InZ, const FVector4& InW);
    /**
     * @brief 16개의 값을 행·열 순서대로 저장합니다.
     * @param f00 0행 0열 값입니다.
     * @param f01 0행 1열 값입니다.
     * @param f02 0행 2열 값입니다.
     * @param f03 0행 3열 값입니다.
     * @param f10 1행 0열 값입니다.
     * @param f11 1행 1열 값입니다.
     * @param f12 1행 2열 값입니다.
     * @param f13 1행 3열 값입니다.
     * @param f20 2행 0열 값입니다.
     * @param f21 2행 1열 값입니다.
     * @param f22 2행 2열 값입니다.
     * @param f23 2행 3열 값입니다.
     * @param f30 3행 0열 값입니다.
     * @param f31 3행 1열 값입니다.
     * @param f32 3행 2열 값입니다.
     * @param f33 3행 3열 값입니다.
     */
    FMatrix(
        const float& f00, const float& f01, const float& f02, const float& f03,
        const float& f10, const float& f11, const float& f12, const float& f13,
        const float& f20, const float& f21, const float& f22, const float& f23,
        const float& f30, const float& f31, const float& f32, const float& f33);

public:
    /**
     * @brief 첫 세 행의 X, Y, Z 축을 정규화해 반환합니다.
     * @param[out] X 첫 번째 행의 방향이며 W는 0입니다.
     * @param[out] Y 두 번째 행의 방향이며 W는 0입니다.
     * @param[out] Z 세 번째 행의 방향이며 W는 0입니다.
     * @note 축 길이가 1e-8 이하이면 해당 방향 성분은 정규화되지 않습니다.
     */
    void GetUnitAxis(FVector4& X, FVector4& Y, FVector4& Z) const;
    /**
     * @brief 지정한 열을 벡터로 반환합니다.
     * @param Index 0부터 시작하는 열 인덱스입니다.
     * @pre Index는 0 이상 4 미만이어야 하며 범위를 검사하지 않습니다.
     * @return 위에서 아래 순서의 열 성분입니다.
     */
    FVector4 GetColumn(int32 Index) const;
    /**
     * @brief 지정한 행의 첫 세 성분을 축 벡터로 설정합니다.
     * @param i 0부터 시작하는 행 인덱스입니다.
     * @param Axis 행에 저장할 X, Y, Z 값입니다.
     * @pre i는 0 이상 4 미만이어야 하며 범위를 검사하지 않습니다.
     * @note 해당 행의 네 번째 성분은 유지합니다.
     */
    void SetAxis(int32 i, const FVector& Axis);
    /**
     * @brief 지정한 열의 네 성분을 설정합니다.
     * @param Index 0부터 시작하는 열 인덱스입니다.
     * @param Value 위에서 아래 순서로 저장할 열 성분입니다.
     * @pre Index는 0 이상 4 미만이어야 하며 범위를 검사하지 않습니다.
     */
    void SetColumn(int32 Index, FVector4 Value);
    //XMMATRIX FMatrixToXMMatrix() const;

    /**
     * @brief 4차원 행 벡터에 현재 행렬을 곱합니다.
     * @param V 변환할 X, Y, Z, W 값입니다.
     * @return V * this에 해당하는 4차원 결과입니다. W 나눗셈은 수행하지 않습니다.
     */
    FVector4 TransformFVector4(const FVector4& V) const;
    /**
     * @brief W를 1로 취급해 위치에 선형 변환과 이동을 적용합니다.
     * @param Vector 변환할 위치입니다.
     * @return 변환 결과의 X, Y, Z입니다. 투영 후 W 나눗셈은 수행하지 않습니다.
     */
    FVector TransformPosition(const FVector& Vector) const;
    /**
     * @brief 행과 열을 교환한 행렬을 반환합니다.
     * @return 현재 행렬의 전치입니다.
     */
    FMatrix GetTransposed() const;
    /**
     * @brief 현재 행렬의 역행렬을 계산합니다.
     * @return 역행렬입니다. 행렬식의 절댓값이 1e-8 이하이면 단위 행렬입니다.
     * @see TryInverse
     */
    FMatrix Inverse() const;
    /**
     * @brief 현재 행렬과 Other의 행렬 곱을 계산합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return this * Other입니다. 행 벡터 변환에서는 현재 변환 후 Other가 적용됩니다.
     */
    FMatrix Multiply(const FMatrix& Other) const;

    /**
     * @brief 현재 4×4 행렬의 행렬식을 계산합니다.
     * @return 행렬식입니다.
     */
    float Determinant() const;

    /**
     * @brief 현재 행렬을 단위 행렬로 설정합니다.
     */
    void SetIdentity();
    /**
     * @brief Inverse와 같은 역행렬 결과를 반환합니다.
     * @return 역행렬입니다. 행렬식의 절댓값이 1e-8 이하이면 단위 행렬입니다.
     */
    FMatrix Invert() const;
    /**
     * @brief 축별 배율로 스케일 행렬을 생성합니다.
     * @param X X축 배율입니다.
     * @param Y Y축 배율입니다.
     * @param Z Z축 배율입니다.
     * @return 지정한 배율을 대각 성분에 저장한 행렬입니다.
     */
    static FMatrix CreateScale(float X, float Y, float Z);
    /**
     * @brief 벡터 성분을 축별 배율로 사용하는 스케일 행렬을 생성합니다.
     * @param Scale X, Y, Z축 배율입니다.
     * @return 스케일 행렬입니다.
     */
    static FMatrix CreateScale(const FVector& Scale);
    /**
     * @brief 이동 행렬을 생성합니다.
     * @param X X축 이동량입니다.
     * @param Y Y축 이동량입니다.
     * @param Z Z축 이동량입니다.
     * @return 네 번째 행에 이동량을 저장한 행렬입니다.
     */
    static FMatrix CreateTranslation(float X, float Y, float Z);
    /**
     * @brief 벡터의 성분으로 이동 행렬을 생성합니다.
     * @param Position X, Y, Z축 이동량입니다.
     * @return 이동 행렬입니다.
     */
    static FMatrix CreateTranslation(const FVector& Position);
    /**
     * @brief X축 주위의 회전 행렬을 생성합니다.
     * @param Radians 라디안 단위 회전 각도입니다.
     * @return X축 회전 행렬입니다.
     */
    static FMatrix CreateRotationX(float Radians);
    /**
     * @brief Y축 주위의 회전 행렬을 생성합니다.
     * @param Radians 라디안 단위 회전 각도입니다.
     * @return Y축 회전 행렬입니다.
     */
    static FMatrix CreateRotationY(float Radians);
    /**
     * @brief Z축 주위의 회전 행렬을 생성합니다.
     * @param Radians 라디안 단위 회전 각도입니다.
     * @return Z축 회전 행렬입니다.
     */
    static FMatrix CreateRotationZ(float Radians);
    /**
     * @brief 라디안 회전 각도로 회전 행렬을 생성합니다.
     * @param Yaw Z축 회전 각도입니다.
     * @param Pitch Y축 회전 각도입니다.
     * @param Roll X축 회전 각도입니다.
     * @return X축 Roll, Y축 Pitch, Z축 Yaw 행렬을 순서대로 곱한 결과입니다.
     * @note FRotator의 도 단위 값은 먼저 라디안으로 변환해야 합니다.
     */
    static FMatrix CreateFromYawPitchRoll(float Yaw, float Pitch, float Roll);
    /**
     * @brief 방향에 현재 행렬의 3×3 선형 변환 부분을 적용합니다.
     * @param Direction 변환할 방향입니다.
     * @return 이동을 제외한 변환 결과입니다. 결과를 정규화하지 않습니다.
     */
    FVector TransformDirection(const FVector& Direction) const;
    /**
     * @brief 네 번째 행의 이동 성분을 반환합니다.
     * @return M[3][0], M[3][1], M[3][2] 값입니다.
     */
    FVector Translation() const;
    /**
     * @brief 네 번째 행의 첫 세 성분에 이동량을 저장합니다.
     * @param Position 저장할 이동량입니다.
     * @note 나머지 행렬 성분은 유지합니다.
     */
    void Translation(const FVector& Position);
    /**
     * @brief 두 번째 행의 Y축 방향을 반환합니다.
     * @return Right 축입니다. 스케일을 포함하며 정규화하지 않습니다.
     */
    FVector Right() const;
    /**
     * @brief 세 번째 행의 Z축 방향을 반환합니다.
     * @return Up 축입니다. 스케일을 포함하며 정규화하지 않습니다.
     */
    FVector Up() const;
    /**
     * @brief 첫 번째 행의 X축 방향을 반환합니다.
     * @return Forward 축입니다. 스케일을 포함하며 정규화하지 않습니다.
     */
    FVector Forward() const;
    /**
     * @brief 양의 Z 방향을 보는 왼손 좌표계 원근 투영 행렬을 생성합니다.
     * @param FovY 라디안 단위 수직 시야각입니다.
     * @param AspectRatio 가로 길이를 세로 길이로 나눈 화면 비율입니다.
     * @param NearZ 가까운 클리핑 평면 거리입니다.
     * @param FarZ 먼 클리핑 평면 거리입니다.
     * @pre FovY는 0과 원주율 사이이고 AspectRatio는 양수이며 0 < NearZ < FarZ이어야 합니다.
     * @return W 나눗셈 후 가까운 평면과 먼 평면의 깊이가 각각 0과 1이 되는 투영 행렬입니다.
     */
    static FMatrix CreatePerspectiveFieldOfView(float FovY, float AspectRatio, float NearZ, float FarZ);
    /**
     * @brief 위치를 W가 1인 행 벡터로 변환하고 결과를 W로 나눕니다.
     * @param Position 변환할 위치입니다.
     * @param[out] OutPosition 성공 시 저장할 변환 결과입니다. 실패 시 기존 값을 유지합니다.
     * @return 변환된 W의 절댓값이 1e-6 이하이면 false이고, 그 외에는 true입니다.
     */
    bool TransformCoord(const FVector& Position, FVector& OutPosition) const;
    /**
     * @brief 행렬식으로 역행렬 계산 가능 여부를 확인하고 결과를 저장합니다.
     * @param[out] OutInverse 성공 시 저장할 역행렬입니다. 실패 시 기존 값을 유지합니다.
     * @return 행렬식의 절댓값이 1e-8 이하이면 false이고, 그 외에는 true입니다.
     */
    bool TryInverse(FMatrix& OutInverse) const;
    /**
     * @brief 양의 Z 방향을 보는 왼손 좌표계 직교 투영 행렬을 생성합니다.
     * @param Width 투영 영역의 가로 길이입니다.
     * @param Height 투영 영역의 세로 길이입니다.
     * @param NearZ 가까운 클리핑 평면 거리입니다.
     * @param FarZ 먼 클리핑 평면 거리입니다.
     * @pre Width와 Height는 양수이고 NearZ는 FarZ보다 작아야 합니다.
     * @return 가까운 평면과 먼 평면의 깊이가 각각 0과 1이 되는 투영 행렬입니다.
     */
    static FMatrix CreateOrthographic(float Width, float Height, float NearZ, float FarZ);
    /**
     * @brief 쿼터니언을 정규화한 뒤 회전 행렬을 생성합니다.
     * @param Rotation 변환할 쿼터니언입니다.
     * @return 이동과 스케일을 포함하지 않는 회전 행렬입니다.
     */
    static FMatrix CreateFromQuaternion(const FQuat& Rotation);
    /**
     * @brief 모든 행렬 성분을 DirectX SimpleMath 행렬로 복사합니다.
     * @return 같은 행·열 값을 갖는 행렬입니다.
     */
    DirectX::SimpleMath::Matrix ToSimpleMath() const;
    /**
     * @brief 회전 부분을 쿼터니언으로 변환합니다.
     * @pre 첫 세 행의 회전 부분에서 스케일과 전단이 제거되어 있어야 합니다.
     * @return FQuat::CreateFromRotationMatrix로 추출한 쿼터니언입니다.
     */
    FQuat ToQuaternion() const;
    /**
     * @brief 첫 세 축의 길이와 방향으로 스케일, 회전과 이동을 분해합니다.
     * @param[out] OutScale 각 축의 길이입니다. 실패하는 경우에도 갱신합니다.
     * @param[out] OutRotation 성공 시 정규화한 축에서 추출한 회전입니다.
     * @param[out] OutTranslation 성공 시 네 번째 행에서 추출한 이동량입니다.
     * @return 축 길이 중 하나라도 1e-6 이하이면 false이고, 그 외에는 true입니다.
     * @note 실패 시 OutRotation과 OutTranslation을 유지합니다. 스케일은 음수 부호를 보존하지 않으며 전단이나 반사 여부를 검사하지 않습니다.
     */
    bool Decompose(FVector& OutScale, FQuat& OutRotation, FVector& OutTranslation) const;

    //void To3x4MatrixTranspose(float* Out) const;
    // FMatrix TransposeAdjoint();

    //GetFrustum Planes Function//

    /* Statics */

    //static XMMATRIX FMatrixToXMMatrix(const FMatrix& M);
    //static FMatrix XMMatrixToFMatrix(const XMMATRIX& Matrix);

    /* Operator */

    /**
     * @brief 모든 행렬 성분의 부호를 반전한 새 행렬을 반환합니다.
     * @return 원본을 변경하지 않은 부호 반전 결과입니다.
     */
    FMatrix operator-();

    /**
     * @brief 지정한 행의 읽기 전용 성분 포인터를 반환합니다.
     * @param Index 0부터 시작하는 행 인덱스입니다.
     * @pre Index는 0 이상 4 미만이어야 하며 범위를 검사하지 않습니다.
     * @return 해당 행의 네 float 성분을 가리키는 포인터입니다.
     */
    const float* operator[](int32 Index) const;
    /**
     * @brief 지정한 행의 수정 가능한 성분 포인터를 반환합니다.
     * @param Index 0부터 시작하는 행 인덱스입니다.
     * @pre Index는 0 이상 4 미만이어야 하며 범위를 검사하지 않습니다.
     * @return 해당 행의 네 float 성분을 가리키는 포인터입니다.
     */
    float* operator[](int32 Index);

    /**
     * @brief 성분별 절대 허용 오차 1e-4로 두 행렬을 비교합니다.
     * @param Other 비교할 행렬입니다.
     * @return 허용 오차 비교에서 같은 행렬로 판단하지 못하면 true입니다.
     */
    bool operator!=(const FMatrix& Other) const;
    /**
     * @brief 성분별 절대 허용 오차 1e-4로 두 행렬을 비교합니다.
     * @param Other 비교할 행렬입니다.
     * @return 모든 성분의 차이가 허용 오차 이하이면 true입니다.
     */
    bool operator==(const FMatrix& Other) const;

    /**
     * @brief 현재 행렬과 Other의 행렬 곱을 반환합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return Multiply와 같은 결과입니다.
     */
    FMatrix operator*(const FMatrix& Other) const;
    /**
     * @brief 모든 행렬 성분에 스칼라를 곱한 결과를 반환합니다.
     * @param Other 곱할 스칼라입니다.
     * @return 성분별 곱셈 결과입니다.
     */
    FMatrix operator*(const float& Other) const;
    /**
     * @brief Other를 행 벡터로 취급해 현재 행렬로 변환합니다.
     * @param Other 변환할 4차원 벡터입니다.
     * @return TransformFVector4와 같은 결과입니다. 표기 순서와 관계없이 Other * this 규약으로 계산합니다.
     */
    FVector4 operator*(const FVector4& Other) const;

    /**
     * @brief 현재 행렬을 Other와 곱한 결과로 교체합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 수정된 현재 행렬의 참조입니다.
     */
    FMatrix& operator*=(const FMatrix& Other);
    /**
     * @brief 모든 성분에 스칼라를 곱해 현재 행렬에 저장합니다.
     * @param Other 곱할 스칼라입니다.
     * @return 수정된 현재 행렬의 참조입니다.
     */
    FMatrix& operator*=(float Other);

    /**
     * @brief 대응하는 행렬 성분을 더한 결과를 반환합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 성분별 연산 결과입니다.
     */
    FMatrix operator+(const FMatrix& Other) const;
    /**
     * @brief 대응하는 성분을 더한 결과를 현재 행렬에 저장합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 수정된 현재 행렬의 참조입니다.
     */
    FMatrix& operator+=(const FMatrix& Other);
    /**
     * @brief 대응하는 행렬 성분을 뺀 결과를 반환합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 성분별 연산 결과입니다.
     */
    FMatrix operator-(const FMatrix& Other) const;
    /**
     * @brief 대응하는 성분을 뺀 결과를 현재 행렬에 저장합니다.
     * @param Other 오른쪽 피연산자입니다.
     * @return 수정된 현재 행렬의 참조입니다.
     */
    FMatrix& operator-=(const FMatrix& Other);

    /**
     * @brief 대각 성분이 1이고 나머지 성분이 0인 단위 행렬 상수입니다.
     */
    static const FMatrix Identity;
};

/* Global Operator*/
/**
 * @brief 행렬의 네 행을 공백으로 성분을 구분해 스트림에 출력합니다.
 * @param OS 출력 스트림입니다.
 * @param M 출력할 행렬입니다.
 * @return OS의 참조입니다.
 */
std::ostream& operator<<(std::ostream& OS, const FMatrix& M);
