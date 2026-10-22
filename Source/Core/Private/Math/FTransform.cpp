#include "Core/Math/FTransform.h"
#include "Core/Math/FMatrix.h"

FTransform::FTransform(const FVector& Location, const FQuat& Rotation, const FVector& Scale)
	: mLocation{Location},
	  mRotation{Rotation},
	  mScale{Scale} {
}

FTransform FTransform::operator*(const FTransform& Other) const {
    return {Other.TransformPosition(mLocation), (Other.mRotation * mRotation).Normalized(), mScale * Other.mScale};
}

FTransform& FTransform::operator*=(const FTransform& Other) {
    *this = *this * Other;

    return *this;
}

FVector FTransform::TransformPosition(const FVector& Position) const {
    return TransformPositionNoScale(mScale * Position);
}

FVector FTransform::TransformVector(const FVector& Vector) const {
    return TransformVectorNoScale(mScale * Vector);
}

FVector FTransform::InverseTransformPosition(const FVector& Position) const {
    return InverseTransformPositionNoScale(Position) / mScale;
}

FVector FTransform::InverseTransformVector(const FVector& Vector) const {
    return InverseTransformVectorNoScale(Vector) / mScale;
}

FVector FTransform::TransformPositionNoScale(const FVector& Position) const {
    return TransformVectorNoScale(Position) + mLocation;
}

FVector FTransform::TransformVectorNoScale(const FVector& Vector) const {
    return mRotation * Vector;
}

FVector FTransform::InverseTransformPositionNoScale(const FVector& Position) const {
    return InverseTransformVectorNoScale(Position - mLocation);
}

FVector FTransform::InverseTransformVectorNoScale(const FVector& Vector) const {
    return mRotation.UnrotateVector(Vector);
}

FTransform FTransform::Blend(const FTransform& Other, float Alpha) const {
    return {mLocation.Lerp(Other.mLocation, Alpha), mRotation.Slerp(Other.mRotation, Alpha), mScale.Lerp(Other.mScale, Alpha)};
}

bool FTransform::NormalizeRotation() {
    return mRotation.Normalize();
}

bool FTransform::IsIdentity(float Tolerance) const {
    return Equals(FTransform{}, Tolerance);
}

bool FTransform::Equals(const FTransform& Other, float Tolerance) const {
    return mLocation.Equals(Other.mLocation, Tolerance) && mRotation.Equals(Other.mRotation, Tolerance) && mScale.Equals(Other.mScale, Tolerance);
}

FRotator FTransform::GetRotator() const {
    return mRotation.Rotator();
}

bool FTransform::SetRotator(const FRotator& Rotation) {
    if (!Rotation.IsFinite()) {
        return false;
    }

    mRotation = Rotation.Quaternion();

    return true;
}

FMatrix FTransform::ToMatrix() const {
    return FMatrix::FromTransform(*this);
}

bool FTransform::IsValid() const {
    const FVector Scale{mScale.Abs()};

    return mLocation.IsFinite() && mScale.IsFinite() && mRotation.IsNormalized() && Scale.x >= 0.0001f && Scale.y >= 0.0001f && Scale.z >= 0.0001f;
}
