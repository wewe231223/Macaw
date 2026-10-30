/**
 * @file FTransform.cpp
 * @brief 위치, 쿼터니언 회전과 축별 스케일을 묶은 공간 변환입니다.
 * @ingroup Core
 */

#include "Core/Math/FTransform.h"
#include "Core/Math/FMatrix.h"

namespace Core {

    /**
     * @brief FTransform의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FTransform::FTransform(const Core::FVector& Location, const Core::FQuat& Rotation, const Core::FVector& Scale)
		: mLocation{Location},
		  mRotation{Rotation},
		  mScale{Scale} {
    }

    /**
     * @brief 현재 변환 뒤에 상대 변환을 적용하도록 위치, 회전과 축별 스케일을 합성합니다.
     */
    Core::FTransform Core::FTransform::operator*(const Core::FTransform& Other) const {
        return {Other.TransformPosition(mLocation), (Other.mRotation * mRotation).Normalized(), mScale * Other.mScale};
    }

    /**
     * @brief 현재 변환 뒤에 상대 변환을 적용하도록 위치, 회전과 축별 스케일을 합성합니다.
     */
    Core::FTransform& Core::FTransform::operator*=(const Core::FTransform& Other) {
        *this = *this * Other;

        return *this;
    }

    /**
     * @brief 위치에 스케일, 회전과 이동을 적용합니다.
     */
    Core::FVector Core::FTransform::TransformPosition(const Core::FVector& Position) const {
        return TransformPositionNoScale(mScale * Position);
    }

    /**
     * @brief 방향 벡터에 스케일과 회전을 적용합니다.
     */
    Core::FVector Core::FTransform::TransformVector(const Core::FVector& Vector) const {
        return TransformVectorNoScale(mScale * Vector);
    }

    /**
     * @brief 월드 위치를 이 변환의 로컬 공간 위치로 되돌립니다.
     */
    Core::FVector Core::FTransform::InverseTransformPosition(const Core::FVector& Position) const {
        return InverseTransformPositionNoScale(Position) / mScale;
    }

    /**
     * @brief 방향 벡터에 역회전과 역스케일을 적용하며 이동은 무시합니다.
     */
    Core::FVector Core::FTransform::InverseTransformVector(const Core::FVector& Vector) const {
        return InverseTransformVectorNoScale(Vector) / mScale;
    }

    /**
     * @brief 위치에 스케일을 제외한 회전과 이동을 적용합니다.
     */
    Core::FVector Core::FTransform::TransformPositionNoScale(const Core::FVector& Position) const {
        return TransformVectorNoScale(Position) + mLocation;
    }

    /**
     * @brief 벡터에 스케일을 제외한 회전만 적용합니다.
     */
    Core::FVector Core::FTransform::TransformVectorNoScale(const Core::FVector& Vector) const {
        return mRotation * Vector;
    }

    /**
     * @brief 위치에 이동과 회전의 역변환을 적용합니다.
     */
    Core::FVector Core::FTransform::InverseTransformPositionNoScale(const Core::FVector& Position) const {
        return InverseTransformVectorNoScale(Position - mLocation);
    }

    /**
     * @brief 벡터에 역회전을 적용합니다.
     */
    Core::FVector Core::FTransform::InverseTransformVectorNoScale(const Core::FVector& Vector) const {
        return mRotation.UnrotateVector(Vector);
    }

    /**
     * @brief 위치·스케일 선형 보간과 회전 보간을 결합한 변환을 구합니다.
     */
    Core::FTransform Core::FTransform::Blend(const Core::FTransform& Other, float Alpha) const {
        return {mLocation.Lerp(Other.mLocation, Alpha), mRotation.Slerp(Other.mRotation, Alpha), mScale.Lerp(Other.mScale, Alpha)};
    }

    /**
     * @brief 변환의 쿼터니언 회전을 제자리에서 정규화합니다.
     */
    bool Core::FTransform::NormalizeRotation() {
        return mRotation.Normalize();
    }

    /**
     * @brief 현재 변환이 허용 오차 이내에서 항등 변환인지 검사합니다.
     */
    bool Core::FTransform::IsIdentity(float Tolerance) const {
        return Equals(Core::FTransform{}, Tolerance);
    }

    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     */
    bool Core::FTransform::Equals(const Core::FTransform& Other, float Tolerance) const {
        return mLocation.Equals(Other.mLocation, Tolerance) && mRotation.Equals(Other.mRotation, Tolerance) && mScale.Equals(Other.mScale, Tolerance);
    }

    /**
     * @brief 현재 쿼터니언 회전을 도 단위 오일러 각으로 변환한 값을 반환합니다.
     */
    Core::FRotator Core::FTransform::GetRotator() const {
        return mRotation.Rotator();
    }

    /**
     * @brief 유효한 도 단위 오일러 회전을 쿼터니언으로 변환하여 적용합니다.
     */
    bool Core::FTransform::SetRotator(const Core::FRotator& Rotation) {
        if (!Rotation.IsFinite()) {
            return false;
        }

        mRotation = Rotation.Quaternion();

        return true;
    }

    /**
     * @brief 위치, 회전과 스케일을 결합한 변환 행렬을 반환합니다.
     */
    Core::FMatrix Core::FTransform::ToMatrix() const {
        return Core::FMatrix::FromTransform(*this);
    }

    /**
     * @brief 성분이 유한하고 회전과 스케일이 사용 가능한 공간 변환인지 검사합니다.
     */
    bool Core::FTransform::IsValid() const {
        const Core::FVector Scale{mScale.Abs()};

        return mLocation.IsFinite() && mScale.IsFinite() && mRotation.IsNormalized() && Scale.x >= 0.0001f && Scale.y >= 0.0001f && Scale.z >= 0.0001f;
    }

}
