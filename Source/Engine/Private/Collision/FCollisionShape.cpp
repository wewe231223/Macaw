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

    /**
     * @brief 축별 반길이로 상자 충돌 형상을 만듭니다.
     */
    Engine::FCollisionShape Engine::FCollisionShape::MakeBox(const Core::FVector& HalfExtent) {
        Engine::FCollisionShape Box{};
        Box.mShapeType = Engine::ECollisionShape::Box;
        Box.mBoxHalfExtent = HalfExtent;

        return Box;
    }

    /**
     * @brief 반지름으로 구 충돌 형상을 만듭니다.
     */
    Engine::FCollisionShape Engine::FCollisionShape::MakeSphere(float Radius) {
        Engine::FCollisionShape Sphere{};
        Sphere.mShapeType = Engine::ECollisionShape::Sphere;
        Sphere.mRadius = Radius;

        return Sphere;
    }

    /**
     * @brief 반지름과 전체 반높이로 캡슐 충돌 형상을 만듭니다.
     */
    Engine::FCollisionShape Engine::FCollisionShape::MakeCapsule(float Radius, float HalfHeight) {
        Engine::FCollisionShape Capsule{};
        Capsule.mShapeType = Engine::ECollisionShape::Capsule;
        Capsule.mRadius = Radius;
        Capsule.mHalfHeight = HalfHeight;

        return Capsule;
    }

    /**
     * @brief 상자 충돌 형상인지 검사합니다.
     */
    bool Engine::FCollisionShape::IsBox() const {
        return mShapeType == Engine::ECollisionShape::Box;
    }

    /**
     * @brief 구 충돌 형상인지 검사합니다.
     */
    bool Engine::FCollisionShape::IsSphere() const {
        return mShapeType == Engine::ECollisionShape::Sphere;
    }

    /**
     * @brief 캡슐 충돌 형상인지 검사합니다.
     */
    bool Engine::FCollisionShape::IsCapsule() const {
        return mShapeType == Engine::ECollisionShape::Capsule;
    }

    /**
     * @brief 반구를 제외한 캡슐 중심 선분 반길이를 반환합니다.
     */
    float Engine::FCollisionShape::GetCapsuleAxisHalfLength() const {
        return std::max(0.0f, mHalfHeight - mRadius);
    }

    /**
     * @brief 충돌 캡슐을 월드 중심 선분과 반지름으로 변환합니다.
     */
    Engine::FCapsuleSegment Engine::Collision::ToCapsuleSegment(const Engine::FCollisionShape& Shape, const Core::FVector& Pos, const Core::FQuat& Rot) {
        Core::FVector Temp{Rot.UpVector() * Shape.GetCapsuleAxisHalfLength()};

        return Engine::FCapsuleSegment{Pos - Temp, Pos + Temp, Shape.GetRadius()};
    }

    /**
     * @brief 로컬 상자를 월드 중심·기저·반길이로 변환합니다.
     */
    Engine::FOrientedBox Engine::Collision::ToOrientedBox(const Engine::FCollisionShape& Shape, const Core::FVector& Pos, const Core::FQuat& Rot) {
        return Engine::FOrientedBox{Pos, {Rot.ForwardVector(), Rot.RightVector(), Rot.UpVector()}, Shape.GetBoxHalfExtent()};
    }

    /**
     * @brief 형상 조합별 좁은 단계 겹침 검사를 수행합니다.
     */
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

    /**
     * @brief 두 캡슐 중심 선분의 거리로 겹침과 접촉을 계산합니다.
     */
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

    /**
     * @brief 캡슐 선분과 회전 상자의 거리로 겹침을 계산합니다.
     */
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

    /**
     * @brief 분리 축 검사로 두 회전 상자의 겹침을 계산합니다.
     */
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

    /**
     * @brief 점에서 선분까지의 최근접 위치를 계산합니다.
     */
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

    /**
     * @brief 두 선분의 최근접 점과 그 사이의 제곱 거리를 계산합니다.
     */
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

    /**
     * @brief 점을 회전 상자의 각 축 범위로 제한한 최근접 위치를 계산합니다.
     */
    Core::FVector Engine::Collision::ClosestPointOnBox(const Core::FVector& P, const Engine::FOrientedBox& Box) {
        const Core::FVector D{P - Box.GetCenter()};

        Core::FVector Result{Box.GetCenter()};

        for (Core::Int32 Index{0}; Index < 3; ++Index) {
            const float S{std::clamp(D.Dot(Box.GetAxis()[Index]), -Box.GetExtent().ToArray()[Index], Box.GetExtent().ToArray()[Index])};

            Result += Box.GetAxis()[Index] * S;
        }

        return Result;
    }

    /**
     * @fn Engine::FSweepResult Engine::Collision::Sweep(const Engine::FCollisionShape& Shape, const Core::FVector& Start, const Core::FVector& Delta, const Core::FQuat& Rot, const Core::TArray<Engine::FSweepTarget>& Targets)
     * @brief 형상을 이동시켜 대상 목록 중 최초 차단 접촉을 찾습니다.
     */
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

    /**
     * @brief 상자의 로컬 축별 반길이를 반환합니다.
     */
    const Core::FVector& Engine::FCollisionShape::GetBoxHalfExtent() const {
        return mBoxHalfExtent;
    }

    /**
     * @brief 충돌 형상의 반지름을 반환합니다.
     */
    float Engine::FCollisionShape::GetRadius() const {
        return mRadius;
    }

    /**
     * @brief 반구를 포함한 캡슐 반높이를 반환합니다.
     */
    float Engine::FCollisionShape::GetHalfHeight() const {
        return mHalfHeight;
    }

    /**
     * @brief 충돌면의 단위 법선을 반환합니다.
     */
    const Core::FVector& Engine::FCollisionContact::GetNormal() const {
        return mNormal;
    }

    /**
     * @brief 충돌면의 단위 법선을 설정합니다.
     */
    void Engine::FCollisionContact::SetNormal(const Core::FVector& Value) {
        mNormal = Value;
    }

    /**
     * @brief 초기 겹침을 해소할 침투 깊이를 반환합니다.
     */
    float Engine::FCollisionContact::GetPenetrationDepth() const {
        return mPenetrationDepth;
    }

    /**
     * @brief 초기 겹침을 해소할 침투 깊이를 설정합니다.
     */
    void Engine::FCollisionContact::SetPenetrationDepth(float Value) {
        mPenetrationDepth = Value;
    }

    /**
     * @brief 월드 접촉점을 반환합니다.
     */
    const Core::FVector& Engine::FCollisionContact::GetPoint() const {
        return mPoint;
    }

    /**
     * @brief 월드 접촉점을 설정합니다.
     */
    void Engine::FCollisionContact::SetPoint(const Core::FVector& Value) {
        mPoint = Value;
    }

    /**
     * @brief 전달된 설정과 기본값으로 FCapsuleSegment 객체를 생성합니다.
     */
    Engine::FCapsuleSegment::FCapsuleSegment(Core::FVector InA, Core::FVector InB, float InRadius)
		: mA{InA},
		  mB{InB},
		  mRadius{InRadius} {
    }

    /**
     * @brief 캡슐 중심 선분의 시작점을 반환합니다.
     */
    const Core::FVector& Engine::FCapsuleSegment::GetA() const {
        return mA;
    }

    /**
     * @brief 캡슐 중심 선분의 시작점을 설정합니다.
     */
    void Engine::FCapsuleSegment::SetA(const Core::FVector& Value) {
        mA = Value;
    }

    /**
     * @brief 캡슐 중심 선분의 끝점을 반환합니다.
     */
    const Core::FVector& Engine::FCapsuleSegment::GetB() const {
        return mB;
    }

    /**
     * @brief 캡슐 중심 선분의 끝점을 설정합니다.
     */
    void Engine::FCapsuleSegment::SetB(const Core::FVector& Value) {
        mB = Value;
    }

    /**
     * @brief 충돌 형상의 반지름을 반환합니다.
     */
    float Engine::FCapsuleSegment::GetRadius() const {
        return mRadius;
    }

    /**
     * @brief 충돌 형상의 반지름을 설정합니다.
     */
    void Engine::FCapsuleSegment::SetRadius(float Value) {
        mRadius = Value;
    }

    /**
     * @brief 전달된 설정과 기본값으로 FOrientedBox 객체를 생성합니다.
     */
    Engine::FOrientedBox::FOrientedBox(Core::FVector InCenter, std::array<Core::FVector, 3> InAxis, Core::FVector InExtent)
		: mCenter{InCenter},
		  mAxis{InAxis},
		  mExtent{InExtent} {
    }

    /**
     * @brief 월드 중심 위치를 반환합니다.
     */
    const Core::FVector& Engine::FOrientedBox::GetCenter() const {
        return mCenter;
    }

    /**
     * @brief 월드 중심 위치를 설정합니다.
     */
    void Engine::FOrientedBox::SetCenter(const Core::FVector& Value) {
        mCenter = Value;
    }

    /**
     * @brief 회전 상자의 세 기저 축을 반환합니다.
     */
    const std::array<Core::FVector, 3>& Engine::FOrientedBox::GetAxis() const {
        return mAxis;
    }

    /**
     * @brief 회전 상자의 세 기저 축을 설정합니다.
     */
    void Engine::FOrientedBox::SetAxis(const std::array<Core::FVector, 3>& Value) {
        mAxis = Value;
    }

    /**
     * @brief 각 축의 반길이를 반환합니다.
     */
    const Core::FVector& Engine::FOrientedBox::GetExtent() const {
        return mExtent;
    }

    /**
     * @brief 각 축의 반길이를 설정합니다.
     */
    void Engine::FOrientedBox::SetExtent(const Core::FVector& Value) {
        mExtent = Value;
    }

    /**
     * @brief 전달된 설정과 기본값으로 FSweepTarget 객체를 생성합니다.
     */
    Engine::FSweepTarget::FSweepTarget(Engine::FCollisionShape InShape, Core::FVector InPos, Core::FQuat InRot)
		: mShape{InShape},
		  mPos{InPos},
		  mRot{InRot} {
    }

    /**
     * @brief 충돌 형상을 반환합니다.
     */
    const Engine::FCollisionShape& Engine::FSweepTarget::GetShape() const {
        return mShape;
    }

    /**
     * @brief 충돌 형상을 설정합니다.
     */
    void Engine::FSweepTarget::SetShape(const Engine::FCollisionShape& Value) {
        mShape = Value;
    }

    /**
     * @brief 월드 위치를 반환합니다.
     */
    const Core::FVector& Engine::FSweepTarget::GetPos() const {
        return mPos;
    }

    /**
     * @brief 월드 위치를 설정합니다.
     */
    void Engine::FSweepTarget::SetPos(const Core::FVector& Value) {
        mPos = Value;
    }

    /**
     * @brief 월드 회전을 반환합니다.
     */
    const Core::FQuat& Engine::FSweepTarget::GetRot() const {
        return mRot;
    }

    /**
     * @brief 월드 회전을 설정합니다.
     */
    void Engine::FSweepTarget::SetRot(const Core::FQuat& Value) {
        mRot = Value;
    }

    /**
     * @brief 이동을 차단하는 충돌 여부를 반환합니다.
     */
    bool Engine::FSweepResult::GetBlockingHit() const {
        return mBlockingHit;
    }

    /**
     * @brief 이동을 차단하는 충돌 여부를 설정합니다.
     */
    void Engine::FSweepResult::SetBlockingHit(bool Value) {
        mBlockingHit = Value;
    }

    /**
     * @brief 추적 시작부터 겹친 상태를 반환합니다.
     */
    bool Engine::FSweepResult::GetStartPenetrating() const {
        return mStartPenetrating;
    }

    /**
     * @brief 추적 시작부터 겹친 상태를 설정합니다.
     */
    void Engine::FSweepResult::SetStartPenetrating(bool Value) {
        mStartPenetrating = Value;
    }

    /**
     * @brief 전체 추적 거리에서 적중한 비율을 반환합니다.
     */
    float Engine::FSweepResult::GetTime() const {
        return mTime;
    }

    /**
     * @brief 전체 추적 거리에서 적중한 비율을 설정합니다.
     */
    void Engine::FSweepResult::SetTime(float Value) {
        mTime = Value;
    }

    /**
     * @brief 적중한 스윕 대상의 인덱스를 반환합니다.
     */
    Core::Int32 Engine::FSweepResult::GetTargetIndex() const {
        return mTargetIndex;
    }

    /**
     * @brief 적중한 스윕 대상의 인덱스를 설정합니다.
     */
    void Engine::FSweepResult::SetTargetIndex(Core::Int32 Value) {
        mTargetIndex = Value;
    }

    /**
     * @brief 충돌 접촉 정보를 반환합니다.
     */
    const Engine::FCollisionContact& Engine::FSweepResult::GetContact() const {
        return mContact;
    }

    /**
     * @brief 충돌 접촉 정보를 설정합니다.
     */
    void Engine::FSweepResult::SetContact(const Engine::FCollisionContact& Value) {
        mContact = Value;
    }

}
