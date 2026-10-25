#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FQuat.h"

enum class ECollisionShape : Uint8 {
    None,
    Box,
    Sphere,
    Capsule
};

ReflectEnum(ECollisionShape, None, Box, Sphere, Capsule);

struct FCollisionShape {
public:
    ReflectTypes(FCollisionShape);

public:
    ReflectStruct(FCollisionShape);
    static FCollisionShape MakeBox(const FVector& HalfExtent);
    static FCollisionShape MakeSphere(float Radius);
    static FCollisionShape MakeCapsule(float Radius, float HalfHeight);
    bool IsBox() const;
    bool IsSphere() const;
    bool IsCapsule() const;
    float GetCapsuleAxisHalfLength() const;
    const FVector& GetBoxHalfExtent() const;
    float GetRadius() const;
    float GetHalfHeight() const;

private:
    ReflectMembers();

private:
    ECollisionShape mShapeType{ECollisionShape::None};
    ReflectProperty(mShapeType, "ShapeType", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mBoxHalfExtent{};
    ReflectProperty(mBoxHalfExtent, "BoxHalfExtent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mRadius{};
    ReflectProperty(mRadius, "Radius", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mHalfHeight{};
    ReflectProperty(mHalfHeight, "HalfHeight", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

struct FCollisionContact {
public:
    ReflectTypes(FCollisionContact);

public:
    ReflectStruct(FCollisionContact);
    const FVector& GetNormal() const;
    void SetNormal(const FVector& Value);
    float GetPenetrationDepth() const;
    void SetPenetrationDepth(float Value);
    const FVector& GetPoint() const;
    void SetPoint(const FVector& Value);

private:
    ReflectMembers();

private:
    FVector mNormal{};
    ReflectProperty(mNormal, "Normal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mPenetrationDepth{};
    ReflectProperty(mPenetrationDepth, "PenetrationDepth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mPoint{};
    ReflectProperty(mPoint, "Point", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

struct FCapsuleSegment {
public:
    ReflectTypes(FCapsuleSegment);

public:
    FCapsuleSegment() = default;
    FCapsuleSegment(FVector InA, FVector InB, float InRadius);

public:
    ReflectStruct(FCapsuleSegment);
    const FVector& GetA() const;
    void SetA(const FVector& Value);
    const FVector& GetB() const;
    void SetB(const FVector& Value);
    float GetRadius() const;
    void SetRadius(float Value);

private:
    ReflectMembers();

private:
    FVector mA{};
    ReflectProperty(mA, "A", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mB{};
    ReflectProperty(mB, "B", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mRadius{};
    ReflectProperty(mRadius, "Radius", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

struct FOrientedBox {
public:
    ReflectTypes(FOrientedBox);

public:
    FOrientedBox() = default;
    FOrientedBox(FVector InCenter, std::array<FVector, 3> InAxis, FVector InExtent);

public:
    ReflectStruct(FOrientedBox);
    const FVector& GetCenter() const;
    void SetCenter(const FVector& Value);
    const std::array<FVector, 3>& GetAxis() const;
    void SetAxis(const std::array<FVector, 3>& Value);
    const FVector& GetExtent() const;
    void SetExtent(const FVector& Value);

private:
    ReflectMembers();

private:
    FVector mCenter{};
    ReflectProperty(mCenter, "Center", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<FVector, 3> mAxis{};
    ReflectProperty(mAxis, "Axis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mExtent{};
    ReflectProperty(mExtent, "Extent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

struct FSweepTarget {
public:
    ReflectTypes(FSweepTarget);

public:
    FSweepTarget() = default;
    FSweepTarget(FCollisionShape InShape, FVector InPos, FQuat InRot);

public:
    ReflectStruct(FSweepTarget);
    const FCollisionShape& GetShape() const;
    void SetShape(const FCollisionShape& Value);
    const FVector& GetPos() const;
    void SetPos(const FVector& Value);
    const FQuat& GetRot() const;
    void SetRot(const FQuat& Value);

private:
    ReflectMembers();

private:
    FCollisionShape mShape{};
    ReflectProperty(mShape, "Shape", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mPos{};
    ReflectProperty(mPos, "Pos", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FQuat mRot{};
    ReflectProperty(mRot, "Rot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

struct FSweepResult {
public:
    ReflectTypes(FSweepResult);

public:
    ReflectStruct(FSweepResult);
    bool GetBlockingHit() const;
    void SetBlockingHit(bool Value);
    bool GetStartPenetrating() const;
    void SetStartPenetrating(bool Value);
    float GetTime() const;
    void SetTime(float Value);
    Int32 GetTargetIndex() const;
    void SetTargetIndex(Int32 Value);
    const FCollisionContact& GetContact() const;
    void SetContact(const FCollisionContact& Value);

private:
    ReflectMembers();

private:
    bool mBlockingHit{};
    ReflectProperty(mBlockingHit, "BlockingHit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mStartPenetrating{};
    ReflectProperty(mStartPenetrating, "StartPenetrating", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mTime{1.0f};
    ReflectProperty(mTime, "Time", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Int32 mTargetIndex{-1};
    ReflectProperty(mTargetIndex, "TargetIndex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FCollisionContact mContact{};
    ReflectProperty(mContact, "Contact", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

namespace Collision {
    FCapsuleSegment ToCapsuleSegment(const FCollisionShape& Shape, const FVector& Pos, const FQuat& Rot);
    FOrientedBox ToOrientedBox(const FCollisionShape& Shape, const FVector& Pos, const FQuat& Rot);

    bool Overlap(const FCollisionShape& A, const FVector& PosA, const FQuat& RotA, const FCollisionShape& B, const FVector& PosB, const FQuat& RotB, FCollisionContact* OutContact = nullptr);

    bool OverlapCapsuleCapsule(const FCapsuleSegment& A, const FCapsuleSegment& B, FCollisionContact* OutContact);
    bool OverlapCapsuleBox(const FCapsuleSegment& Capsule, const FOrientedBox& Box, FCollisionContact* OutContact);
    bool OverlapBoxBox(const FOrientedBox& A, const FOrientedBox& B, FCollisionContact* OutContact);

    FVector ClosestPointOnSegment(const FVector& P, const FVector& A, const FVector& B);
    float ClosestPointsSegmentSegment(const FVector& P1, const FVector& Q1, const FVector& P2, const FVector& Q2, FVector& OutC1, FVector& OutC2);
    FVector ClosestPointOnBox(const FVector& P, const FOrientedBox& Box);

    FSweepResult Sweep(const FCollisionShape& Shape, const FVector& Start, const FVector& Delta, const FQuat& Rot, const TArray<FSweepTarget>& Targets);
}
