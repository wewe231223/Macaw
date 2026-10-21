#pragma once

#include "Core/CoreMinimal.h"
#include "Math/FMath.h"
#include "Core/Archive/FArchive.h"

struct FTransform {
public:
    FTransform() = default;
    FTransform(const FVector3& InPosition, const FRotator& InRotation, const FVector3& InScale);
    FTransform(const FVector3& InPosition, const FQuat& InRotation, const FVector3& InScale);

public:
    bool operator==(const FTransform& Other) const;
    const FVector3& GetPosition() const;
    FRotator GetRotation() const;
    const FQuat& GetRotationQuaternion() const;
    const FVector3& GetScale() const;
    void SetPosition(const FVector3& InPosition);
    void SetRotation(const FRotator& InRotation);
    void SetRotation(const FQuat& InRotation);
    void SetScale(const FVector3& InScale);
    FMatrix ToMatrixWithScale() const;
    FMatrix ToMatrixNoScale() const;
    FMatrix ToInverseMatrixWithScale() const;
    FTransform Compose(const FTransform& Parent) const;
    bool MakeRelativeTo(const FTransform& Parent, FTransform& OutRelative) const;
    void Serialize(FArchive& Archive);

private:
    FVector3 mPosition{};
    FQuat mRotation{};
    FVector3 mScale{1.0f, 1.0f, 1.0f};
};
