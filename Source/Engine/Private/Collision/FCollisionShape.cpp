/**
 * @file FCollisionShape.cpp
 * @brief 구·상자·캡슐의 로컬 치수를 담는 충돌 형상 값입니다.
 * @ingroup Engine
 */

#include "Engine/Collision/FCollisionShape.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

using namespace Collision;

FCollisionShape FCollisionShape::MakeBox(const FVector& HalfExtent) {
    FCollisionShape Box{};
    Box.mShapeType = ECollisionShape::Box;
    Box.mBoxHalfExtent = HalfExtent;
    return Box;
}

FCollisionShape FCollisionShape::MakeSphere(float Radius) {
    FCollisionShape Sphere{};
    Sphere.mShapeType = ECollisionShape::Sphere;
    Sphere.mRadius = Radius;
    return Sphere;
}

FCollisionShape FCollisionShape::MakeCapsule(float Radius, float HalfHeight) {
    FCollisionShape Capsule{};
    Capsule.mShapeType = ECollisionShape::Capsule;
    Capsule.mRadius = Radius;
    Capsule.mHalfHeight = HalfHeight;
    return Capsule;
}

bool FCollisionShape::IsBox() const {
    return mShapeType == ECollisionShape::Box;
}

bool FCollisionShape::IsSphere() const {
    return mShapeType == ECollisionShape::Sphere;
}

bool FCollisionShape::IsCapsule() const {
    return mShapeType == ECollisionShape::Capsule;
}

float FCollisionShape::GetCapsuleAxisHalfLength() const {
    return std::max(0.0f, mHalfHeight - mRadius);
}

FCapsuleSegment Collision::ToCapsuleSegment(const FCollisionShape& Shape, const FVector& Pos, const FQuat& Rot) {
    FVector Temp{Rot.UpVector() * Shape.GetCapsuleAxisHalfLength()};
    return FCapsuleSegment{Pos - Temp, Pos + Temp, Shape.GetRadius()};
}

FOrientedBox Collision::ToOrientedBox(const FCollisionShape& Shape, const FVector& Pos, const FQuat& Rot) {
    return FOrientedBox{Pos, {Rot.ForwardVector(), Rot.RightVector(), Rot.UpVector()}, Shape.GetBoxHalfExtent()};
}

bool Collision::Overlap(const FCollisionShape& A, const FVector& PosA, const FQuat& RotA, const FCollisionShape& B, const FVector& PosB, const FQuat& RotB, FCollisionContact* OutContact) {
    if ((A.IsSphere() || A.IsCapsule()) && (B.IsSphere() || B.IsCapsule())) {
        FCapsuleSegment CapA{Collision::ToCapsuleSegment(A, PosA, RotA)};
        FCapsuleSegment CapB{Collision::ToCapsuleSegment(B, PosB, RotB)};

        return OverlapCapsuleCapsule(CapA, CapB, OutContact);
    }
    else if ((A.IsSphere() || A.IsCapsule()) && B.IsBox()) {
        FCapsuleSegment CapA{Collision::ToCapsuleSegment(A, PosA, RotA)};
        FOrientedBox BoxB{Collision::ToOrientedBox(B, PosB, RotB)};

        return OverlapCapsuleBox(CapA, BoxB, OutContact);
    }
    else if (A.IsBox() && (B.IsSphere() || B.IsCapsule())) {
        FOrientedBox BoxA{Collision::ToOrientedBox(A, PosA, RotA)};
        FCapsuleSegment CapB{Collision::ToCapsuleSegment(B, PosB, RotB)};

        const bool Overlap{OverlapCapsuleBox(CapB, BoxA, OutContact)};
        if (Overlap && OutContact) {
            OutContact->SetNormal(-OutContact->GetNormal());
            OutContact->SetPoint(OutContact->GetPoint() + OutContact->GetNormal() * OutContact->GetPenetrationDepth());
        }
        return Overlap;
    }
    else if (A.IsBox() && B.IsBox()) {
        FOrientedBox BoxA{Collision::ToOrientedBox(A, PosA, RotA)};
        FOrientedBox BoxB{Collision::ToOrientedBox(B, PosB, RotB)};

        return OverlapBoxBox(BoxA, BoxB, OutContact);
    }

    return false;
}

bool Collision::OverlapCapsuleCapsule(const FCapsuleSegment& A, const FCapsuleSegment& B, FCollisionContact* OutContact) {
    FVector C1{};
    FVector C2{};
    float SumR{A.GetRadius() + B.GetRadius()};
    float DistSq{ClosestPointsSegmentSegment(A.GetA(), A.GetB(), B.GetA(), B.GetB(), C1, C2)};
    bool IsOverlap{DistSq <= SumR * SumR};
    if (IsOverlap && OutContact) {
        constexpr float Epsilon{1e-6f};
        FVector D{C1 - C2};
        float Len{D.Length()};
        FVector Normal{0.0f, 0.0f, 1.0f};
        if (Len > Epsilon) {
            Normal = D / Len;
        }
        else {
            const FVector CenterDiff{(A.GetA() + A.GetB() - B.GetA() - B.GetB()) * 0.5f};
            const FVector Perp{(A.GetB() - A.GetA()).Cross(B.GetB() - B.GetA())};
            const float PerpLen{Perp.Length()};
            const float CenterLen{CenterDiff.Length()};
            if (PerpLen > Epsilon)
                Normal = (Perp.Dot(CenterDiff) < 0.0f ? -Perp : Perp) / PerpLen;
            else if (CenterLen > Epsilon)
                Normal = CenterDiff / CenterLen;
        }

        OutContact->SetNormal(Normal);
        OutContact->SetPenetrationDepth(A.GetRadius() + B.GetRadius() - Len);
        OutContact->SetPoint(C2 + Normal * B.GetRadius());
    }

    return IsOverlap;
}

bool Collision::OverlapCapsuleBox(const FCapsuleSegment& Capsule, const FOrientedBox& Box, FCollisionContact* OutContact) {
    const FVector Dir{Capsule.GetB() - Capsule.GetA()};
    const auto DistSqAt{[&](float T) {
        const FVector P{Capsule.GetA() + Dir * T};
        const FVector D{P - ClosestPointOnBox(P, Box)};
        return D.Dot(D);
    }};

    float Lo{0.0f};
    float Hi{1.0f};
    for (Int32 Iter{0}; Iter < 30; ++Iter) {
        const float M1{Lo + (Hi - Lo) / 3.0f};
        const float M2{Hi - (Hi - Lo) / 3.0f};
        if (DistSqAt(M1) <= DistSqAt(M2))
            Hi = M2;
        else
            Lo = M1;
    }

    const FVector P{Capsule.GetA() + Dir * ((Lo + Hi) * 0.5f)};
    const FVector Q{ClosestPointOnBox(P, Box)};
    const FVector D{P - Q};
    const float DistSq{D.Dot(D)};
    const bool Overlap{DistSq <= Capsule.GetRadius() * Capsule.GetRadius()};
    if (!Overlap || !OutContact)
        return Overlap;

    const float Len{std::sqrt(DistSq)};
    if (Len > 1e-6f) {
        OutContact->SetNormal(D / Len);
        OutContact->SetPenetrationDepth(Capsule.GetRadius() - Len);
        OutContact->SetPoint(Q);
        return true;
    }

    const FVector Local{P - Box.GetCenter()};
    Int32 MinAxis{0};
    float MinGap{FLT_MAX};
    float MinCoord{0.0f};
    for (Int32 Index{0}; Index < 3; ++Index) {
        const float S{Local.Dot(Box.GetAxis()[Index])};
        const float Gap{Box.GetExtent().ToArray()[Index] - std::abs(S)};
        if (Gap < MinGap) {
            MinGap = Gap;
            MinAxis = Index;
            MinCoord = S;
        }
    }

    OutContact->SetNormal(Box.GetAxis()[MinAxis] * (MinCoord >= 0.0f ? 1.0f : -1.0f));
    OutContact->SetPenetrationDepth(Capsule.GetRadius() + MinGap);
    OutContact->SetPoint(P + OutContact->GetNormal() * MinGap);
    return true;
}

bool Collision::OverlapBoxBox(const FOrientedBox& A, const FOrientedBox& B, FCollisionContact* OutContact) {
    constexpr float Epsilon{1e-6f};

    float R[3][3]{};
    float AbsR[3][3]{};
    for (Int32 Index{0}; Index < 3; ++Index) {
        for (Int32 OtherIndex{0}; OtherIndex < 3; ++OtherIndex) {
            R[Index][OtherIndex] = A.GetAxis()[Index].Dot(B.GetAxis()[OtherIndex]);
            AbsR[Index][OtherIndex] = std::abs(R[Index][OtherIndex]) + Epsilon;
        }
    }

    const FVector Offset{B.GetCenter() - A.GetCenter()};
    const float T[3]{ Offset.Dot(A.GetAxis()[0]), Offset.Dot(A.GetAxis()[1]), Offset.Dot(A.GetAxis()[2]) };

    const FVector& EA{A.GetExtent()};
    const FVector& EB{B.GetExtent()};

    float MinOverlap{FLT_MAX};
    FVector MinAxis{A.GetAxis()[0]};
    const auto TrackMin{[&](float Overlap, const FVector& Axis) {
        if (OutContact && Overlap < MinOverlap) {
            MinOverlap = Overlap;
            MinAxis = Axis;
        }
    }};

    for (Int32 Index{0}; Index < 3; ++Index) {
        const float RA{EA.ToArray()[Index]};
        const float RB{EB.ToArray()[0] * AbsR[Index][0] + EB.ToArray()[1] * AbsR[Index][1] + EB.ToArray()[2] * AbsR[Index][2]};
        if (std::abs(T[Index]) > RA + RB)
            return false;
        TrackMin(RA + RB - std::abs(T[Index]), A.GetAxis()[Index]);
    }

    for (Int32 OtherIndex{0}; OtherIndex < 3; ++OtherIndex) {
        const float RA{EA.ToArray()[0] * AbsR[0][OtherIndex] + EA.ToArray()[1] * AbsR[1][OtherIndex] + EA.ToArray()[2] * AbsR[2][OtherIndex]};
        const float RB{EB.ToArray()[OtherIndex]};
        const float Dist{T[0] * R[0][OtherIndex] + T[1] * R[1][OtherIndex] + T[2] * R[2][OtherIndex]};
        if (std::abs(Dist) > RA + RB)
            return false;
        TrackMin(RA + RB - std::abs(Dist), B.GetAxis()[OtherIndex]);
    }

    for (Int32 Index{0}; Index < 3; ++Index) {
        const Int32 I1{(Index + 1) % 3};
        const Int32 I2{(Index + 2) % 3};
        for (Int32 OtherIndex{0}; OtherIndex < 3; ++OtherIndex) {
            const Int32 J1{(OtherIndex + 1) % 3};
            const Int32 J2{(OtherIndex + 2) % 3};

            const float RA{EA.ToArray()[I1] * AbsR[I2][OtherIndex] + EA.ToArray()[I2] * AbsR[I1][OtherIndex]};
            const float RB{EB.ToArray()[J1] * AbsR[Index][J2] + EB.ToArray()[J2] * AbsR[Index][J1]};
            const float Dist{T[I2] * R[I1][OtherIndex] - T[I1] * R[I2][OtherIndex]};
            if (std::abs(Dist) > RA + RB)
                return false;

            const float AxisLen{std::sqrt(std::max(0.0f, 1.0f - R[Index][OtherIndex] * R[Index][OtherIndex]))};
            if (AxisLen > 1e-3f)
                TrackMin((RA + RB - std::abs(Dist)) / AxisLen, A.GetAxis()[Index].Cross(B.GetAxis()[OtherIndex]) / AxisLen);
        }
    }

    if (OutContact) {
        const FVector Normal{MinAxis.Dot(A.GetCenter() - B.GetCenter()) < 0.0f ? -MinAxis : MinAxis};
        OutContact->SetNormal(Normal);
        OutContact->SetPenetrationDepth(MinOverlap);
        OutContact->SetPoint(ClosestPointOnBox(A.GetCenter(), B));
    }

    return true;
}

FVector Collision::ClosestPointOnSegment(const FVector& P, const FVector& A, const FVector& B) {
    FVector BA{B - A};

    if (BA.Dot(BA) < 1e-8f) {
        return A;
    }

    FVector PA{P - A};

    float T{PA.Dot(BA) / BA.Dot(BA)};
    T = std::clamp(T, 0.0f, 1.0f);

    FVector C{A + BA * T};

    return C;
}

float Collision::ClosestPointsSegmentSegment(const FVector& P1, const FVector& Q1, const FVector& P2, const FVector& Q2, FVector& OutC1, FVector& OutC2) {
    FVector D1{Q1 - P1};
    FVector D2{Q2 - P2};
    FVector R{P1 - P2};

    float A{D1.Dot(D1)};
    float E{D2.Dot(D2)};
    float F{D2.Dot(R)};

    if (A < 1e-8f && E < 1e-8f) {
        OutC1 = P1;
        OutC2 = P2;
        return R.Dot(R);
    }
    else if (A < 1e-8f) {
        OutC1 = P1;
        OutC2 = P2 + D2 * std::clamp(F / E, 0.0f, 1.0f);
        FVector D{OutC2 - OutC1};
        return D.Dot(D);
    }
    else if (E < 1e-8f) {
        float C{D1.Dot(R)};
        OutC1 = P1 + D1 * std::clamp(-C / A, 0.0f, 1.0f);
        OutC2 = P2;
        FVector D{OutC2 - OutC1};
        return D.Dot(D);
    }
    else {
        float B{D1.Dot(D2)};
        float C{D1.Dot(R)};
        float Denom{A * E - B * B};

        float S{Denom > 1e-8f ? std::clamp((B * F - C * E) / Denom, 0.0f, 1.0f) : 0.0f};
        float T{(B * S + F) / E};

        if (T < 0.0f) {
            T = 0.0f;
            S = std::clamp(-C / A, 0.0f, 1.0f);
        }
        else if (T > 1.0f) {
            T = 1.0f;
            S = std::clamp((B - C) / A, 0.0f, 1.0f);
        }

        OutC1 = P1 + D1 * S;
        OutC2 = P2 + D2 * T;
        FVector D{OutC2 - OutC1};
        return D.Dot(D);
    }
}

FVector Collision::ClosestPointOnBox(const FVector& P, const FOrientedBox& Box) {
    const FVector D{P - Box.GetCenter()};

    FVector Result{Box.GetCenter()};
    for (Int32 Index{0}; Index < 3; ++Index) {
        const float S{std::clamp(D.Dot(Box.GetAxis()[Index]), -Box.GetExtent().ToArray()[Index], Box.GetExtent().ToArray()[Index])};
        Result += Box.GetAxis()[Index] * S;
    }
    return Result;
}

FSweepResult Collision::Sweep(const FCollisionShape& Shape, const FVector& Start, const FVector& Delta, const FQuat& Rot, const TArray<FSweepTarget>& Targets) {
    FSweepResult Result{};

    const float DeltaLen{Delta.Length()};
    if (DeltaLen < 1e-8f || Targets.size() == 0)
        return Result;

    TArray<Uint8> Ignored{};
    Ignored.resize(Targets.size());

    const auto FirstOverlapAt{[&](float T, FCollisionContact* OutContact) -> Int32 {
        const FVector Pos{Start + Delta * T};
        for (Int32 Index{0}; Index < static_cast<Int32>(Targets.size()); ++Index) {
            if (Ignored[Index])
                continue;
            const FSweepTarget& Target{Targets[Index]};
            if (Overlap(Shape, Pos, Rot, Target.GetShape(), Target.GetPos(), Target.GetRot(), OutContact))
                return Index;
        }
        return -1;
    }};

    for (Int32 Index{0}; Index < static_cast<Int32>(Targets.size()); ++Index) {
        FCollisionContact Contact{};
        const FSweepTarget& Target{Targets[Index]};
        if (!Overlap(Shape, Start, Rot, Target.GetShape(), Target.GetPos(), Target.GetRot(), &Contact))
            continue;

        if (Delta.Dot(Contact.GetNormal()) >= 0.0f) {
            Ignored[Index] = 1;
            continue;
        }

        Result.SetBlockingHit(true);
        Result.SetStartPenetrating(true);
        Result.SetTime(0.0f);
        Result.SetTargetIndex(Index);
        Result.SetContact(Contact);
        return Result;
    }

    float MinHalfSize{Shape.GetRadius()};
    if (Shape.IsBox())
        MinHalfSize = std::min({ Shape.GetBoxHalfExtent().x, Shape.GetBoxHalfExtent().y, Shape.GetBoxHalfExtent().z });
    const Int32 Steps{MinHalfSize > 1e-4f ? std::clamp(static_cast<Int32>(std::ceil(DeltaLen / MinHalfSize)), 1, 64) : 1};

    for (Int32 Step{0}; Step < Steps; ++Step) {
        float Lo{static_cast<float>(Step) / Steps};
        float Hi{static_cast<float>(Step + 1) / Steps};
        if (FirstOverlapAt(Hi, nullptr) < 0)
            continue;

        for (Int32 Iter{0}; Iter < 10; ++Iter) {
            const float Mid{(Lo + Hi) * 0.5f};
            if (FirstOverlapAt(Mid, nullptr) >= 0)
                Hi = Mid;
            else
                Lo = Mid;
        }

        Result.SetBlockingHit(true);
        FCollisionContact Contact{};
        Result.SetTargetIndex(FirstOverlapAt(Hi, &Contact));
        Result.SetContact(Contact);
        Result.SetTime(std::max(0.0f, Lo - 1e-3f / DeltaLen));
        return Result;
    }

    return Result;
}

const FVector& FCollisionShape::GetBoxHalfExtent() const {
    return mBoxHalfExtent;
}

float FCollisionShape::GetRadius() const {
    return mRadius;
}

float FCollisionShape::GetHalfHeight() const {
    return mHalfHeight;
}

const FVector& FCollisionContact::GetNormal() const {
    return mNormal;
}

void FCollisionContact::SetNormal(const FVector& Value) {
    mNormal = Value;
}

float FCollisionContact::GetPenetrationDepth() const {
    return mPenetrationDepth;
}

void FCollisionContact::SetPenetrationDepth(float Value) {
    mPenetrationDepth = Value;
}

const FVector& FCollisionContact::GetPoint() const {
    return mPoint;
}

void FCollisionContact::SetPoint(const FVector& Value) {
    mPoint = Value;
}

FCapsuleSegment::FCapsuleSegment(FVector InA, FVector InB, float InRadius)
	: mA{InA},
	  mB{InB},
	  mRadius{InRadius} {
}

const FVector& FCapsuleSegment::GetA() const {
    return mA;
}

void FCapsuleSegment::SetA(const FVector& Value) {
    mA = Value;
}

const FVector& FCapsuleSegment::GetB() const {
    return mB;
}

void FCapsuleSegment::SetB(const FVector& Value) {
    mB = Value;
}

float FCapsuleSegment::GetRadius() const {
    return mRadius;
}

void FCapsuleSegment::SetRadius(float Value) {
    mRadius = Value;
}

FOrientedBox::FOrientedBox(FVector InCenter, std::array<FVector, 3> InAxis, FVector InExtent)
	: mCenter{InCenter},
	  mAxis{InAxis},
	  mExtent{InExtent} {
}

const FVector& FOrientedBox::GetCenter() const {
    return mCenter;
}

void FOrientedBox::SetCenter(const FVector& Value) {
    mCenter = Value;
}

const std::array<FVector, 3>& FOrientedBox::GetAxis() const {
    return mAxis;
}

void FOrientedBox::SetAxis(const std::array<FVector, 3>& Value) {
    mAxis = Value;
}

const FVector& FOrientedBox::GetExtent() const {
    return mExtent;
}

void FOrientedBox::SetExtent(const FVector& Value) {
    mExtent = Value;
}

FSweepTarget::FSweepTarget(FCollisionShape InShape, FVector InPos, FQuat InRot)
	: mShape{InShape},
	  mPos{InPos},
	  mRot{InRot} {
}

const FCollisionShape& FSweepTarget::GetShape() const {
    return mShape;
}

void FSweepTarget::SetShape(const FCollisionShape& Value) {
    mShape = Value;
}

const FVector& FSweepTarget::GetPos() const {
    return mPos;
}

void FSweepTarget::SetPos(const FVector& Value) {
    mPos = Value;
}

const FQuat& FSweepTarget::GetRot() const {
    return mRot;
}

void FSweepTarget::SetRot(const FQuat& Value) {
    mRot = Value;
}

bool FSweepResult::GetBlockingHit() const {
    return mBlockingHit;
}

void FSweepResult::SetBlockingHit(bool Value) {
    mBlockingHit = Value;
}

bool FSweepResult::GetStartPenetrating() const {
    return mStartPenetrating;
}

void FSweepResult::SetStartPenetrating(bool Value) {
    mStartPenetrating = Value;
}

float FSweepResult::GetTime() const {
    return mTime;
}

void FSweepResult::SetTime(float Value) {
    mTime = Value;
}

Int32 FSweepResult::GetTargetIndex() const {
    return mTargetIndex;
}

void FSweepResult::SetTargetIndex(Int32 Value) {
    mTargetIndex = Value;
}

const FCollisionContact& FSweepResult::GetContact() const {
    return mContact;
}

void FSweepResult::SetContact(const FCollisionContact& Value) {
    mContact = Value;
}
