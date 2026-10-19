#include "pch.h"
#include "Core/Base/FTransform.h"

FTransform::FTransform(const FVector3& InPosition, const FRotator& InRotation, const FVector3& InScale)
	: mPosition{InPosition},
	  mRotation{FQuat::FromRotator(InRotation)},
	  mScale{InScale} {
}

FTransform::FTransform(const FVector3& InPosition, const FQuat& InRotation, const FVector3& InScale)
	: mPosition{InPosition},
	  mRotation{InRotation},
	  mScale{InScale} {
    mRotation.Normalize();
}

bool FTransform::operator==(const FTransform& Other) const {
    const bool SameRotation{mRotation.X == Other.mRotation.X && mRotation.Y == Other.mRotation.Y && mRotation.Z == Other.mRotation.Z && mRotation.W == Other.mRotation.W};
    const bool OppositeRotation{mRotation.X == -Other.mRotation.X && mRotation.Y == -Other.mRotation.Y && mRotation.Z == -Other.mRotation.Z && mRotation.W == -Other.mRotation.W};

    return mPosition == Other.mPosition && mScale == Other.mScale && (SameRotation || OppositeRotation);
}

const FVector3& FTransform::GetPosition() const {
    return mPosition;
}

FRotator FTransform::GetRotation() const {
    return mRotation.ToRotator();
}

const FQuat& FTransform::GetRotationQuaternion() const {
    return mRotation;
}

const FVector3& FTransform::GetScale() const {
    return mScale;
}

void FTransform::SetPosition(const FVector3& InPosition) {
    mPosition = InPosition;
}

void FTransform::SetRotation(const FRotator& InRotation) {
    mRotation = FQuat::FromRotator(InRotation);
}

void FTransform::SetRotation(const FQuat& InRotation) {
    mRotation = InRotation;
    mRotation.Normalize();
}

void FTransform::SetScale(const FVector3& InScale) {
    mScale = InScale;
}

FMatrix FTransform::ToMatrixWithScale() const {
    FMatrix Result{FMatrix::CreateScale(mScale) * FMatrix::CreateFromQuaternion(mRotation)};

    Result.Translation(mPosition);

    return Result;
}

FMatrix FTransform::ToMatrixNoScale() const {
    FMatrix Result{FMatrix::CreateFromQuaternion(mRotation)};

    Result.Translation(mPosition);

    return Result;
}

FMatrix FTransform::ToInverseMatrixWithScale() const {
    return ToMatrixWithScale().Invert();
}

FTransform FTransform::Compose(const FTransform& Parent) const {
    const FVector3 ScaledPosition{mPosition.mX * Parent.mScale.mX, mPosition.mY * Parent.mScale.mY, mPosition.mZ * Parent.mScale.mZ};
    const FVector3 Position{Parent.mRotation.RotateVector(ScaledPosition) + Parent.mPosition};
    const FVector3 Scale{mScale.mX * Parent.mScale.mX, mScale.mY * Parent.mScale.mY, mScale.mZ * Parent.mScale.mZ};

    return FTransform{Position, FQuat::Concatenate(Parent.mRotation, mRotation), Scale};
}

bool FTransform::MakeRelativeTo(const FTransform& Parent, FTransform& OutRelative) const {
    constexpr float Epsilon{1e-6f};

    if (std::abs(Parent.mScale.mX) <= Epsilon || std::abs(Parent.mScale.mY) <= Epsilon || std::abs(Parent.mScale.mZ) <= Epsilon) {
        return false;
    }

    const FVector3 ParentSpacePosition{Parent.mRotation.Inverse().RotateVector(mPosition - Parent.mPosition)};
    const FVector3 Position{ParentSpacePosition.mX / Parent.mScale.mX, ParentSpacePosition.mY / Parent.mScale.mY, ParentSpacePosition.mZ / Parent.mScale.mZ};
    const FQuat Rotation{FQuat::Concatenate(Parent.mRotation.Inverse(), mRotation)};
    const FVector3 Scale{mScale.mX / Parent.mScale.mX, mScale.mY / Parent.mScale.mY, mScale.mZ / Parent.mScale.mZ};

    OutRelative = FTransform{Position, Rotation, Scale};

    return true;
}

void FTransform::Serialize(FArchive& Archive) {
    FVector3 Position{mPosition};
    FQuat Rotation{mRotation};
    FVector3 Scale{mScale};

    Archive.Serialize("Position", Position);
    Archive.Serialize("Rotation", Rotation);
    Archive.Serialize("Scale", Scale);

    if (Archive.IsLoading() && !Archive.HasError()) {
        *this = FTransform{Position, Rotation, Scale};
    }
}
