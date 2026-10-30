/**
 * @file FCollisionShape.cpp
 * @brief 구·상자·캡슐의 로컬 치수를 담는 충돌 형상 값입니다.
 * @ingroup Engine
 */

#include "Engine/Collision/FCollisionShape.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace Engine {

    Engine::FCollisionShape Engine::FCollisionShape::MakeBox(const Core::FVector& HalfExtent) {
        Engine::FCollisionShape Box{};
        Box.mShapeType = Engine::ECollisionShape::Box;
        Box.mBoxHalfExtent = HalfExtent;
        return Box;
    }

    Engine::FCollisionShape Engine::FCollisionShape::MakeSphere(float Radius) {
        Engine::FCollisionShape Sphere{};
        Sphere.mShapeType = Engine::ECollisionShape::Sphere;
        Sphere.mRadius = Radius;
        return Sphere;
    }

    Engine::FCollisionShape Engine::FCollisionShape::MakeCapsule(float Radius, float HalfHeight) {
        Engine::FCollisionShape Capsule{};
        Capsule.mShapeType = Engine::ECollisionShape::Capsule;
        Capsule.mRadius = Radius;
        Capsule.mHalfHeight = HalfHeight;

        return Capsule;
    }

    bool Engine::FCollisionShape::IsBox() const {
        return mShapeType == Engine::ECollisionShape::Box;
    }

    bool Engine::FCollisionShape::IsSphere() const {
        return mShapeType == Engine::ECollisionShape::Sphere;
    }

    bool Engine::FCollisionShape::IsCapsule() const {
        return mShapeType == Engine::ECollisionShape::Capsule;
    }

    float Engine::FCollisionShape::GetCapsuleAxisHalfLength() const {
        return std::max(0.0f, mHalfHeight - mRadius);
    }

    Engine::FCapsuleSegment Engine::Collision::ToCapsuleSegment(const Engine::FCollisionShape& Shape, const Core::FVector& Pos, const Core::FQuat& Rot) {
        Core::FVector Temp{Rot.UpVector() * Shape.GetCapsuleAxisHalfLength()};
        return Engine::FCapsuleSegment{Pos - Temp, Pos + Temp, Shape.GetRadius()};
    }

    Engine::FOrientedBox Engine::Collision::ToOrientedBox(const Engine::FCollisionShape& Shape, const Core::FVector& Pos, const Core::FQuat& Rot) {
        return Engine::FOrientedBox{Pos, {Rot.ForwardVector(), Rot.RightVector(), Rot.UpVector()}, Shape.GetBoxHalfExtent()};
    }

    bool Engine::Collision::Overlap(const Engine::FCollisionShape& A, const Core::FVector& PosA, const Core::FQuat& RotA, const Engine::FCollisionShape& B, const Core::FVector& PosB, const Core::FQuat& RotB, Engine::FCollisionContact* OutContact) {
        if ((A.IsSphere() || A.IsCapsule()) && (B.IsSphere() || B.IsCapsule())) {
            Engine::FCapsuleSegment CapA{Engine::Collision::ToCapsuleSegment(A, PosA, RotA)};
            Engine::FCapsuleSegment CapB{Engine::Collision::ToCapsuleSegment(B, PosB, RotB)};

            return Engine::Collision::OverlapCapsuleCapsule(CapA, CapB, OutContact);
        }
        else if ((A.IsSphere() || A.IsCapsule()) && B.IsBox()) {
            Engine::FCapsuleSegment CapA{Engine::Collision::ToCapsuleSegment(A, PosA, RotA)};
            Engine::FOrientedBox BoxB{Engine::Collision::ToOrientedBox(B, PosB, RotB)};

            return Engine::Collision::OverlapCapsuleBox(CapA, BoxB, OutContact);
        }
        else if (A.IsBox() && (B.IsSphere() || B.IsCapsule())) {
            Engine::FOrientedBox BoxA{Engine::Collision::ToOrientedBox(A, PosA, RotA)};
            Engine::FCapsuleSegment CapB{Engine::Collision::ToCapsuleSegment(B, PosB, RotB)};

            const bool Overlap{Engine::Collision::OverlapCapsuleBox(CapB, BoxA, OutContact)};

            if (Overlap && OutContact) {
                OutContact->SetNormal(-OutContact->GetNormal());
                OutContact->SetPoint(OutContact->GetPoint() + OutContact->GetNormal() * OutContact->GetPenetrationDepth());
            }

            return Overlap;
        }
        else if (A.IsBox() && B.IsBox()) {
            Engine::FOrientedBox BoxA{Engine::Collision::ToOrientedBox(A, PosA, RotA)};
            Engine::FOrientedBox BoxB{Engine::Collision::ToOrientedBox(B, PosB, RotB)};

            return Engine::Collision::OverlapBoxBox(BoxA, BoxB, OutContact);
        }

        return false;
    }

    bool Engine::Collision::OverlapCapsuleCapsule(const Engine::FCapsuleSegment& A, const Engine::FCapsuleSegment& B, Engine::FCollisionContact* OutContact) {
        Core::FVector C1{};
        Core::FVector C2{};
        float SumR{A.GetRadius() + B.GetRadius()};
        float DistSq{Engine::Collision::ClosestPointsSegmentSegment(A.GetA(), A.GetB(), B.GetA(), B.GetB(), C1, C2)};
        bool IsOverlap{DistSq <= SumR * SumR};

        if (IsOverlap && OutContact) {
            constexpr float Epsilon{1e-6f};
            Core::FVector D{C1 - C2};
            float Len{D.Length()};
            Core::FVector Normal{0.0f, 0.0f, 1.0f};

            if (Len > Epsilon) {
                Normal = D / Len;
            }
            else {
                const Core::FVector CenterDiff{(A.GetA() + A.GetB() - B.GetA() - B.GetB()) * 0.5f};
                const Core::FVector Perp{(A.GetB() - A.GetA()).Cross(B.GetB() - B.GetA())};
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

    bool Engine::Collision::OverlapCapsuleBox(const Engine::FCapsuleSegment& Capsule, const Engine::FOrientedBox& Box, Engine::FCollisionContact* OutContact) {
        const Core::FVector Dir{Capsule.GetB() - Capsule.GetA()};

        const auto DistSqAt{[&](float T) {
            const Core::FVector P{Capsule.GetA() + Dir * T};
            const Core::FVector D{P - Engine::Collision::ClosestPointOnBox(P, Box)};
            return D.Dot(D);
        }};

        float Lo{0.0f};
        float Hi{1.0f};

        for (Core::Int32 Iter{0}; Iter < 30; ++Iter) {
            const float M1{Lo + (Hi - Lo) / 3.0f};
            const float M2{Hi - (Hi - Lo) / 3.0f};

            if (DistSqAt(M1) <= DistSqAt(M2))
                Hi = M2;
            else
                Lo = M1;
        }

        const Core::FVector P{Capsule.GetA() + Dir * ((Lo + Hi) * 0.5f)};
        const Core::FVector Q{Engine::Collision::ClosestPointOnBox(P, Box)};
        const Core::FVector D{P - Q};
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

        const Core::FVector Local{P - Box.GetCenter()};
        Core::Int32 MinAxis{0};
        float MinGap{FLT_MAX};
        float MinCoord{0.0f};

        for (Core::Int32 Index{0}; Index < 3; ++Index) {
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

    bool Engine::Collision::OverlapBoxBox(const Engine::FOrientedBox& A, const Engine::FOrientedBox& B, Engine::FCollisionContact* OutContact) {
        constexpr float Epsilon{1e-6f};

        float R[3][3]{};
        float AbsR[3][3]{};

        for (Core::Int32 Index{0}; Index < 3; ++Index) {
            for (Core::Int32 OtherIndex{0}; OtherIndex < 3; ++OtherIndex) {
                R[Index][OtherIndex] = A.GetAxis()[Index].Dot(B.GetAxis()[OtherIndex]);
                AbsR[Index][OtherIndex] = std::abs(R[Index][OtherIndex]) + Epsilon;
            }
        }

        const Core::FVector Offset{B.GetCenter() - A.GetCenter()};
        const float T[3]{ Offset.Dot(A.GetAxis()[0]), Offset.Dot(A.GetAxis()[1]), Offset.Dot(A.GetAxis()[2]) };

        const Core::FVector& EA{A.GetExtent()};
        const Core::FVector& EB{B.GetExtent()};

        float MinOverlap{FLT_MAX};
        Core::FVector MinAxis{A.GetAxis()[0]};

        const auto TrackMin{[&](float Overlap, const Core::FVector& Axis) {
            if (OutContact && Overlap < MinOverlap) {
                MinOverlap = Overlap;
                MinAxis = Axis;
            }
        }};

        for (Core::Int32 Index{0}; Index < 3; ++Index) {
            const float RA{EA.ToArray()[Index]};
            const float RB{EB.ToArray()[0] * AbsR[Index][0] + EB.ToArray()[1] * AbsR[Index][1] + EB.ToArray()[2] * AbsR[Index][2]};

            if (std::abs(T[Index]) > RA + RB)
                return false;
            TrackMin(RA + RB - std::abs(T[Index]), A.GetAxis()[Index]);
        }

        for (Core::Int32 OtherIndex{0}; OtherIndex < 3; ++OtherIndex) {
            const float RA{EA.ToArray()[0] * AbsR[0][OtherIndex] + EA.ToArray()[1] * AbsR[1][OtherIndex] + EA.ToArray()[2] * AbsR[2][OtherIndex]};
            const float RB{EB.ToArray()[OtherIndex]};
            const float Dist{T[0] * R[0][OtherIndex] + T[1] * R[1][OtherIndex] + T[2] * R[2][OtherIndex]};

            if (std::abs(Dist) > RA + RB)
                return false;
            TrackMin(RA + RB - std::abs(Dist), B.GetAxis()[OtherIndex]);
        }

        for (Core::Int32 Index{0}; Index < 3; ++Index) {
            const Core::Int32 I1{(Index + 1) % 3};
            const Core::Int32 I2{(Index + 2) % 3};

            for (Core::Int32 OtherIndex{0}; OtherIndex < 3; ++OtherIndex) {
                const Core::Int32 J1{(OtherIndex + 1) % 3};
                const Core::Int32 J2{(OtherIndex + 2) % 3};

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
            const Core::FVector Normal{MinAxis.Dot(A.GetCenter() - B.GetCenter()) < 0.0f ? -MinAxis : MinAxis};
            OutContact->SetNormal(Normal);
            OutContact->SetPenetrationDepth(MinOverlap);
            OutContact->SetPoint(Engine::Collision::ClosestPointOnBox(A.GetCenter(), B));
        }

        return true;
    }

    Core::FVector Engine::Collision::ClosestPointOnSegment(const Core::FVector& P, const Core::FVector& A, const Core::FVector& B) {
        Core::FVector BA{B - A};

        if (BA.Dot(BA) < 1e-8f) {
            return A;
        }

        Core::FVector PA{P - A};

        float T{PA.Dot(BA) / BA.Dot(BA)};
        T = std::clamp(T, 0.0f, 1.0f);

        Core::FVector C{A + BA * T};

        return C;
    }

    float Engine::Collision::ClosestPointsSegmentSegment(const Core::FVector& P1, const Core::FVector& Q1, const Core::FVector& P2, const Core::FVector& Q2, Core::FVector& OutC1, Core::FVector& OutC2) {
        Core::FVector D1{Q1 - P1};
        Core::FVector D2{Q2 - P2};
        Core::FVector R{P1 - P2};

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

            Core::FVector D{OutC2 - OutC1};
            return D.Dot(D);
        }
        else if (E < 1e-8f) {
            float C{D1.Dot(R)};
            OutC1 = P1 + D1 * std::clamp(-C / A, 0.0f, 1.0f);
            OutC2 = P2;

            Core::FVector D{OutC2 - OutC1};

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

            Core::FVector D{OutC2 - OutC1};

            return D.Dot(D);
        }
    }

    Core::FVector Engine::Collision::ClosestPointOnBox(const Core::FVector& P, const Engine::FOrientedBox& Box) {
        const Core::FVector D{P - Box.GetCenter()};

        Core::FVector Result{Box.GetCenter()};

        for (Core::Int32 Index{0}; Index < 3; ++Index) {
            const float S{std::clamp(D.Dot(Box.GetAxis()[Index]), -Box.GetExtent().ToArray()[Index], Box.GetExtent().ToArray()[Index])};
            Result += Box.GetAxis()[Index] * S;
        }

        return Result;
    }

    Engine::FSweepResult Engine::Collision::Sweep(const Engine::FCollisionShape& Shape, const Core::FVector& Start, const Core::FVector& Delta, const Core::FQuat& Rot, const Core::TArray<Engine::FSweepTarget>& Targets) {
        Engine::FSweepResult Result{};

        const float DeltaLen{Delta.Length()};

        if (DeltaLen < 1e-8f || Targets.size() == 0)
            return Result;

        Core::TArray<Core::Uint8> Ignored{};
        Ignored.resize(Targets.size());

        const auto FirstOverlapAt{[&](float T, Engine::FCollisionContact* OutContact) -> Core::Int32 {
            const Core::FVector Pos{Start + Delta * T};
            for (Core::Int32 Index{0}; Index < static_cast<Core::Int32>(Targets.size()); ++Index) {
                if (Ignored[Index])
                    continue;
                const Engine::FSweepTarget& Target{Targets[Index]};

                if (Engine::Collision::Overlap(Shape, Pos, Rot, Target.GetShape(), Target.GetPos(), Target.GetRot(), OutContact))
                    return Index;
            }
            return -1;
        }};

        for (Core::Int32 Index{0}; Index < static_cast<Core::Int32>(Targets.size()); ++Index) {
            Engine::FCollisionContact Contact{};
            const Engine::FSweepTarget& Target{Targets[Index]};

            if (!Engine::Collision::Overlap(Shape, Start, Rot, Target.GetShape(), Target.GetPos(), Target.GetRot(), &Contact))
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
        const Core::Int32 Steps{MinHalfSize > 1e-4f ? std::clamp(static_cast<Core::Int32>(std::ceil(DeltaLen / MinHalfSize)), 1, 64) : 1};

        for (Core::Int32 Step{0}; Step < Steps; ++Step) {
            float Lo{static_cast<float>(Step) / Steps};
            float Hi{static_cast<float>(Step + 1) / Steps};

            if (FirstOverlapAt(Hi, nullptr) < 0)
                continue;

            for (Core::Int32 Iter{0}; Iter < 10; ++Iter) {
                const float Mid{(Lo + Hi) * 0.5f};

                if (FirstOverlapAt(Mid, nullptr) >= 0)
                    Hi = Mid;
                else
                    Lo = Mid;
            }

            Result.SetBlockingHit(true);

            Engine::FCollisionContact Contact{};
            Result.SetTargetIndex(FirstOverlapAt(Hi, &Contact));
            Result.SetContact(Contact);
            Result.SetTime(std::max(0.0f, Lo - 1e-3f / DeltaLen));

            return Result;
        }

        return Result;
    }

    const Core::FVector& Engine::FCollisionShape::GetBoxHalfExtent() const {
        return mBoxHalfExtent;
    }

    float Engine::FCollisionShape::GetRadius() const {
        return mRadius;
    }

    float Engine::FCollisionShape::GetHalfHeight() const {
        return mHalfHeight;
    }

    const Core::FVector& Engine::FCollisionContact::GetNormal() const {
        return mNormal;
    }

    void Engine::FCollisionContact::SetNormal(const Core::FVector& Value) {
        mNormal = Value;
    }

    float Engine::FCollisionContact::GetPenetrationDepth() const {
        return mPenetrationDepth;
    }

    void Engine::FCollisionContact::SetPenetrationDepth(float Value) {
        mPenetrationDepth = Value;
    }

    const Core::FVector& Engine::FCollisionContact::GetPoint() const {
        return mPoint;
    }

    void Engine::FCollisionContact::SetPoint(const Core::FVector& Value) {
        mPoint = Value;
    }

    Engine::FCapsuleSegment::FCapsuleSegment(Core::FVector InA, Core::FVector InB, float InRadius)
		: mA{InA},
		  mB{InB},
		  mRadius{InRadius} {
    }

    const Core::FVector& Engine::FCapsuleSegment::GetA() const {
        return mA;
    }

    void Engine::FCapsuleSegment::SetA(const Core::FVector& Value) {
        mA = Value;
    }

    const Core::FVector& Engine::FCapsuleSegment::GetB() const {
        return mB;
    }

    void Engine::FCapsuleSegment::SetB(const Core::FVector& Value) {
        mB = Value;
    }

    float Engine::FCapsuleSegment::GetRadius() const {
        return mRadius;
    }

    void Engine::FCapsuleSegment::SetRadius(float Value) {
        mRadius = Value;
    }

    Engine::FOrientedBox::FOrientedBox(Core::FVector InCenter, std::array<Core::FVector, 3> InAxis, Core::FVector InExtent)
		: mCenter{InCenter},
		  mAxis{InAxis},
		  mExtent{InExtent} {
    }

    const Core::FVector& Engine::FOrientedBox::GetCenter() const {
        return mCenter;
    }

    void Engine::FOrientedBox::SetCenter(const Core::FVector& Value) {
        mCenter = Value;
    }

    const std::array<Core::FVector, 3>& Engine::FOrientedBox::GetAxis() const {
        return mAxis;
    }

    void Engine::FOrientedBox::SetAxis(const std::array<Core::FVector, 3>& Value) {
        mAxis = Value;
    }

    const Core::FVector& Engine::FOrientedBox::GetExtent() const {
        return mExtent;
    }

    void Engine::FOrientedBox::SetExtent(const Core::FVector& Value) {
        mExtent = Value;
    }

    Engine::FSweepTarget::FSweepTarget(Engine::FCollisionShape InShape, Core::FVector InPos, Core::FQuat InRot)
		: mShape{InShape},
		  mPos{InPos},
		  mRot{InRot} {
    }

    const Engine::FCollisionShape& Engine::FSweepTarget::GetShape() const {
        return mShape;
    }

    void Engine::FSweepTarget::SetShape(const Engine::FCollisionShape& Value) {
        mShape = Value;
    }

    const Core::FVector& Engine::FSweepTarget::GetPos() const {
        return mPos;
    }

    void Engine::FSweepTarget::SetPos(const Core::FVector& Value) {
        mPos = Value;
    }

    const Core::FQuat& Engine::FSweepTarget::GetRot() const {
        return mRot;
    }

    void Engine::FSweepTarget::SetRot(const Core::FQuat& Value) {
        mRot = Value;
    }

    bool Engine::FSweepResult::GetBlockingHit() const {
        return mBlockingHit;
    }

    void Engine::FSweepResult::SetBlockingHit(bool Value) {
        mBlockingHit = Value;
    }

    bool Engine::FSweepResult::GetStartPenetrating() const {
        return mStartPenetrating;
    }

    void Engine::FSweepResult::SetStartPenetrating(bool Value) {
        mStartPenetrating = Value;
    }

    float Engine::FSweepResult::GetTime() const {
        return mTime;
    }

    void Engine::FSweepResult::SetTime(float Value) {
        mTime = Value;
    }

    Core::Int32 Engine::FSweepResult::GetTargetIndex() const {
        return mTargetIndex;
    }

    void Engine::FSweepResult::SetTargetIndex(Core::Int32 Value) {
        mTargetIndex = Value;
    }

    const Engine::FCollisionContact& Engine::FSweepResult::GetContact() const {
        return mContact;
    }

    void Engine::FSweepResult::SetContact(const Engine::FCollisionContact& Value) {
        mContact = Value;
    }

}
