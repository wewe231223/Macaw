#include "pch.h"
#include "FTransform.h"

FTransform::FTransform(const FVector3& InPosition, const FRotator& InRotation, const FVector3& InScale)
    : mPosition(InPosition),
      mRotation(FQuat::FromRotator(InRotation)),
      mRotationEuler(InRotation),
      mScale(InScale) {
}

FTransform::FTransform(const FVector3& InPosition, const FQuat& InRotation, const FVector3& InScale)
    : mPosition(InPosition),
      mRotation(InRotation),
      mScale(InScale) {
    mRotation.Normalize();
    mRotationEuler = mRotation.ToRotator();
}

const FVector3& FTransform::GetPosition() const {
    return mPosition;
}

const FVector3& FTransform::GetLocation() const {
    return mPosition;
}

const FRotator& FTransform::GetRotation() const {
    return mRotationEuler;
}

const FQuat& FTransform::GetRotationQuaternion() const {
    return mRotation;
}

const FVector3& FTransform::GetScale() const {
    return mScale;
}

const FVector3& FTransform::GetScale3D() const {
    return mScale;
}

bool FTransform::IsAbsoluteLocation() const {
    return mBAbsoluteLocation;
}

bool FTransform::IsAbsoluteRotation() const {
    return mBAbsoluteRotation;
}

bool FTransform::IsAbsoluteScale() const {
    return mBAbsoluteScale;
}

void FTransform::SetPosition(const FVector3& InPosition) {
    mPosition = InPosition;
}

void FTransform::SetLocation(const FVector3& Location) {
    mPosition = Location;
}

void FTransform::SetScale(const FVector3& InScale) {
    mScale = InScale;
}

void FTransform::SetScale3D(const FVector3& Scale) {
    this->mScale = Scale;
}

void FTransform::SetAbsoluteLocation(bool BInAbsoluteLocation) {
    mBAbsoluteLocation = BInAbsoluteLocation;
}

void FTransform::SetAbsoluteRotation(bool BInAbsoluteRotation) {
    mBAbsoluteRotation = BInAbsoluteRotation;
}

void FTransform::SetAbsoluteScale(bool BInAbsoluteScale) {
    mBAbsoluteScale = BInAbsoluteScale;
}
