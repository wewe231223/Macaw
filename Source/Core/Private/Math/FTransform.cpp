/**
 * @file FTransform.cpp
 * @brief 위치, 쿼터니언 회전과 축별 스케일을 묶은 공간 변환입니다.
 * @ingroup Core
 */

#include "Core/Math/FTransform.h"
#include "Core/Math/FMatrix.h"

/**
 * @brief FTransform의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FTransform::FTransform(const FVector& Location, const FQuat& Rotation, const FVector& Scale)
	: mLocation{Location},
	  mRotation{Rotation},
	  mScale{Scale} {
}

/**
 * @brief 현재 변환 뒤에 상대 변환을 적용하도록 위치, 회전과 축별 스케일을 합성합니다.
 */
FTransform FTransform::operator*(const FTransform& Other) const {
    return {Other.TransformPosition(mLocation), (Other.mRotation * mRotation).Normalized(), mScale * Other.mScale};
}

/**
 * @brief 현재 변환 뒤에 상대 변환을 적용하도록 위치, 회전과 축별 스케일을 합성합니다.
 */
FTransform& FTransform::operator*=(const FTransform& Other) {
    *this = *this * Other;

    return *this;
}

/**
 * @brief 위치에 스케일, 회전과 이동을 적용합니다.
 */
FVector FTransform::TransformPosition(const FVector& Position) const {
    return TransformPositionNoScale(mScale * Position);
}

/**
 * @brief 방향 벡터에 스케일과 회전을 적용합니다.
 */
FVector FTransform::TransformVector(const FVector& Vector) const {
    return TransformVectorNoScale(mScale * Vector);
}

/**
 * @brief 위치 또는 벡터에 역변환을 적용합니다.
 */
FVector FTransform::InverseTransformPosition(const FVector& Position) const {
    return InverseTransformPositionNoScale(Position) / mScale;
}

/**
 * @brief 위치 또는 벡터에 역변환을 적용합니다.
 */
FVector FTransform::InverseTransformVector(const FVector& Vector) const {
    return InverseTransformVectorNoScale(Vector) / mScale;
}

/**
 * @brief 위치에 스케일을 제외한 회전과 이동을 적용합니다.
 */
FVector FTransform::TransformPositionNoScale(const FVector& Position) const {
    return TransformVectorNoScale(Position) + mLocation;
}

/**
 * @brief 벡터에 스케일을 제외한 회전만 적용합니다.
 */
FVector FTransform::TransformVectorNoScale(const FVector& Vector) const {
    return mRotation * Vector;
}

/**
 * @brief 위치에 이동과 회전의 역변환을 적용합니다.
 */
FVector FTransform::InverseTransformPositionNoScale(const FVector& Position) const {
    return InverseTransformVectorNoScale(Position - mLocation);
}

/**
 * @brief 벡터에 역회전을 적용합니다.
 */
FVector FTransform::InverseTransformVectorNoScale(const FVector& Vector) const {
    return mRotation.UnrotateVector(Vector);
}

/**
 * @brief 위치·스케일 선형 보간과 회전 보간을 결합한 변환을 구합니다.
 */
FTransform FTransform::Blend(const FTransform& Other, float Alpha) const {
    return {mLocation.Lerp(Other.mLocation, Alpha), mRotation.Slerp(Other.mRotation, Alpha), mScale.Lerp(Other.mScale, Alpha)};
}

/**
 * @brief 변환의 쿼터니언 회전을 제자리에서 정규화합니다.
 */
bool FTransform::NormalizeRotation() {
    return mRotation.Normalize();
}

/**
 * @brief 현재 변환이 허용 오차 이내에서 항등 변환인지 검사합니다.
 */
bool FTransform::IsIdentity(float Tolerance) const {
    return Equals(FTransform{}, Tolerance);
}

/**
 * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
 */
bool FTransform::Equals(const FTransform& Other, float Tolerance) const {
    return mLocation.Equals(Other.mLocation, Tolerance) && mRotation.Equals(Other.mRotation, Tolerance) && mScale.Equals(Other.mScale, Tolerance);
}

/**
 * @brief 현재 쿼터니언 회전을 도 단위 오일러 각으로 변환한 값을 반환합니다.
 */
FRotator FTransform::GetRotator() const {
    return mRotation.Rotator();
}

/**
 * @brief 유효한 도 단위 오일러 회전을 쿼터니언으로 변환하여 적용합니다.
 */
bool FTransform::SetRotator(const FRotator& Rotation) {
    if (!Rotation.IsFinite()) {
        return false;
    }

    mRotation = Rotation.Quaternion();

    return true;
}

/**
 * @brief 위치, 회전과 스케일을 결합한 변환 행렬을 반환합니다.
 */
FMatrix FTransform::ToMatrix() const {
    return FMatrix::FromTransform(*this);
}

/**
 * @brief 성분이 유한하고 회전과 스케일이 사용 가능한 공간 변환인지 검사합니다.
 */
bool FTransform::IsValid() const {
    const FVector Scale{mScale.Abs()};

    return mLocation.IsFinite() && mScale.IsFinite() && mRotation.IsNormalized() && Scale.x >= 0.0001f && Scale.y >= 0.0001f && Scale.z >= 0.0001f;
}
