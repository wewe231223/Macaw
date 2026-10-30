/**
 * @file UShapeComponent.cpp
 * @brief 충돌 형상·접촉 이벤트와 에디터 보조 선을 제공하는 기본 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UShapeComponent.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/EPrimitiveType.h"
#include "Core/Math/FBox.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>

namespace Engine {

    /**
     * @brief 충돌 도형의 접촉 검사와 디버그 형상 생성 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 두 AABB의 모든 축 구간이 겹치는지 검사합니다.
         * @param[in] A 첫 번째 검사 대상입니다.
         * @param[in] B 두 번째 검사 대상입니다.
         * @return 두 형상이 겹치면 true입니다.
         */
        bool BoundsOverlap(const Core::FBox& A, const Core::FBox& B) {
            return A.mMinimum.x <= B.mMaximum.x && A.mMaximum.x >= B.mMinimum.x && A.mMinimum.y <= B.mMaximum.y && A.mMaximum.y >= B.mMinimum.y && A.mMinimum.z <= B.mMaximum.z && A.mMaximum.z >= B.mMinimum.z;
        }

        /**
         * @brief 충돌 도형 원호를 분할할 선분 개수입니다.
         */
        constexpr Core::Int32 CircleSegment{32};

        // 각도는 모두 라디안
        /**
         * @brief 두 기저 방향의 평면에서 원호를 선분으로 수집합니다.
         * @param[in] DrawInterface 보조 선분을 추가할 프레임 수집기입니다.
         * @param[in] C 원호의 중심입니다.
         * @param[in] U 원호 평면의 첫 기저입니다.
         * @param[in] V 원호 평면의 둘째 기저입니다.
         * @param[in] R 원호의 반지름입니다.
         * @param[in] StartAngleRad 시작 라디안 각도입니다.
         * @param[in] EndAngleRad 끝 라디안 각도입니다.
         * @param[in] Color 적용할 색상입니다.
         */
        void AddArc(Engine::FPrimitiveDrawInterface& DrawInterface, const Core::FVector& C, const Core::FVector& U, const Core::FVector& V, float R, float StartAngleRad, float EndAngleRad, const Core::FVector& Color) {
            const Core::Int32 Segments{std::max(1, static_cast<Core::Int32>(CircleSegment * (EndAngleRad - StartAngleRad) / (2.0f * std::numbers::pi_v<float>)))};
            const float StepRad{(EndAngleRad - StartAngleRad) / Segments};

            Core::FVector Prev{C + U * (std::cos(StartAngleRad) * R) + V * (std::sin(StartAngleRad) * R)};

            for (Core::Int32 Index{1}; Index <= Segments; ++Index) {
                const float AngleRad{StartAngleRad + StepRad * Index};
                const Core::FVector Point{C + U * (std::cos(AngleRad) * R) + V * (std::sin(AngleRad) * R)};
                DrawInterface.DrawLine(Prev, Point, Color, true);
                Prev = Point;
            }
        }
    }

    Engine::UShapeComponent::UShapeComponent() {
        mOnComponentBeginOverlap.Add([](Engine::UPrimitiveComponent*, Engine::AActor*, Engine::UPrimitiveComponent*) {
            std::clog << "BeginOverlap\n";
        });

        mOnComponentEndOverlap.Add([](Engine::UPrimitiveComponent*, Engine::AActor*, Engine::UPrimitiveComponent*) {
            std::clog << "EndOverlap\n";
        });
    }

    void Engine::UShapeComponent::OnBeginOverlap(Engine::UShapeComponent* Other) {
        mOnComponentBeginOverlap.Broadcast(this, Other ? Other->GetOwner() : nullptr, Other);
    }

    void Engine::UShapeComponent::OnEndOverlap(Engine::UShapeComponent* Other) {
        mOnComponentEndOverlap.Broadcast(this, Other ? Other->GetOwner() : nullptr, Other);
    }

    void Engine::UShapeComponent::OnHit(Engine::AActor* OtherActor, Engine::UPrimitiveComponent* OtherComp, const Engine::FHitResult& Hit) {
        mOnComponentHit.Broadcast(this, OtherActor, OtherComp, Hit);
    }

    bool Engine::UShapeComponent::ShouldCreateRenderState() const {
        return false;
    }

    void Engine::UShapeComponent::DrawDebug(Engine::FPrimitiveDrawInterface& DrawInterface, bool Highlight) const {
        Engine::FCollisionShape Shape{GetCollisionShape()};
        Core::FVector Pos{GetTransform().mLocation};
        Core::FQuat Rot{GetTransform().mRotation};
        Core::FVector Color{Highlight ? Core::FVector{1.0f, 0.0f, 0.0f} : Core::FVector{mShapeColor.x, mShapeColor.y, mShapeColor.z}};

        if (Shape.IsBox()) {
            Engine::FOrientedBox Box{Engine::Collision::ToOrientedBox(Shape, Pos, Rot)};

            Core::FVector V0{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
            Core::FVector V1{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
            Core::FVector V2{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
            Core::FVector V3{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
            Core::FVector V4{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
            Core::FVector V5{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
            Core::FVector V6{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
            Core::FVector V7{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};

            DrawInterface.DrawLine(V0, V1, Color, true);
            DrawInterface.DrawLine(V1, V2, Color, true);
            DrawInterface.DrawLine(V2, V3, Color, true);
            DrawInterface.DrawLine(V3, V0, Color, true);
            DrawInterface.DrawLine(V4, V5, Color, true);
            DrawInterface.DrawLine(V5, V6, Color, true);
            DrawInterface.DrawLine(V6, V7, Color, true);
            DrawInterface.DrawLine(V7, V4, Color, true);
            DrawInterface.DrawLine(V0, V4, Color, true);
            DrawInterface.DrawLine(V1, V5, Color, true);
            DrawInterface.DrawLine(V2, V6, Color, true);
            DrawInterface.DrawLine(V3, V7, Color, true);
        }
        else if (Shape.IsSphere()) {
            Engine::FCapsuleSegment Sphere{Engine::Collision::ToCapsuleSegment(Shape, Pos, Rot)};

            const Core::FVector F{Rot.ForwardVector()};
            const Core::FVector Rt{Rot.RightVector()};
            const Core::FVector Up{Rot.UpVector()};
            const float R{Sphere.GetRadius()};

            AddArc(DrawInterface, Sphere.GetA(), F, Rt, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);
            AddArc(DrawInterface, Sphere.GetA(), F, Up, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);
            AddArc(DrawInterface, Sphere.GetA(), Rt, Up, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);
        }
        else if (Shape.IsCapsule()) {
            Engine::FCapsuleSegment Capsule{Engine::Collision::ToCapsuleSegment(Shape, Pos, Rot)};

            const Core::FVector F{Rot.ForwardVector()};
            const Core::FVector Rt{Rot.RightVector()};
            const Core::FVector Up{Rot.UpVector()};
            const float R{Capsule.GetRadius()};

            AddArc(DrawInterface, Capsule.GetA(), F, Rt, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);
            AddArc(DrawInterface, Capsule.GetB(), F, Rt, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);

            DrawInterface.DrawLine(Capsule.GetA() + F * R, Capsule.GetB() + F * R, Color, true);
            DrawInterface.DrawLine(Capsule.GetA() - F * R, Capsule.GetB() - F * R, Color, true);
            DrawInterface.DrawLine(Capsule.GetA() + Rt * R, Capsule.GetB() + Rt * R, Color, true);
            DrawInterface.DrawLine(Capsule.GetA() - Rt * R, Capsule.GetB() - Rt * R, Color, true);

            AddArc(DrawInterface, Capsule.GetB(), F, Up, R, 0.0f, std::numbers::pi_v<float>, Color);
            AddArc(DrawInterface, Capsule.GetB(), Rt, Up, R, 0.0f, std::numbers::pi_v<float>, Color);
            AddArc(DrawInterface, Capsule.GetA(), F, -Up, R, 0.0f, std::numbers::pi_v<float>, Color);
            AddArc(DrawInterface, Capsule.GetA(), Rt, -Up, R, 0.0f, std::numbers::pi_v<float>, Color);
        }
    }

    bool Engine::UShapeComponent::IsOverlapping(const Engine::UShapeComponent& Other) const {
        if (&Other == this || !BoundsOverlap(CalcBounds(), Other.CalcBounds())) {
            return false;
        }

        return Engine::Collision::Overlap(GetCollisionShape(), GetTransform().mLocation, GetTransform().mRotation, Other.GetCollisionShape(), Other.GetTransform().mLocation, Other.GetTransform().mRotation);
    }

    bool Engine::UShapeComponent::ComputeContact(const Engine::UShapeComponent& Other, Engine::FCollisionContact& OutContact) const {
        if (&Other == this || !BoundsOverlap(CalcBounds(), Other.CalcBounds())) {
            return false;
        }

        return Engine::Collision::Overlap(GetCollisionShape(), GetTransform().mLocation, GetTransform().mRotation, Other.GetCollisionShape(), Other.GetTransform().mLocation, Other.GetTransform().mRotation, &OutContact);
    }

    bool Engine::UShapeComponent::ShouldDrawOnlyIfSelected() const {
        return mDrawOnlyIfSelected;
    }

    float Engine::UShapeComponent::GetShapeScale() const {
        Core::FVector Scale{GetTransform().mScale};
        return std::min({ std::abs(Scale.x), std::abs(Scale.y), std::abs(Scale.z) });
    }

    Engine::UShapeComponent::FOverlapDelegate& Engine::UShapeComponent::GetOnComponentBeginOverlap() {
        return mOnComponentBeginOverlap;
    }

    Engine::UShapeComponent::FOverlapDelegate& Engine::UShapeComponent::GetOnComponentEndOverlap() {
        return mOnComponentEndOverlap;
    }

    Engine::UShapeComponent::FHitDelegate& Engine::UShapeComponent::GetOnComponentHit() {
        return mOnComponentHit;
    }

    Core::FBox Engine::UShapeComponent::CalcBounds() const {
        return CalcLocalBounds().Transformed(GetWorldMatrix());
    }

    Core::FBox Engine::UShapeComponent::GetWorldBounds() const {
        return CalcBounds();
    }

    Engine::EPrimitiveType Engine::UShapeComponent::GetPrimitiveType() const {
        return Engine::EPrimitiveType::Cube;
    }

    const Core::FVector4& Engine::UShapeComponent::GetShapeColor() const {
        return mShapeColor;
    }

    bool Engine::UShapeComponent::SetShapeColor(const Core::FVector4& Color) {
        if (!std::isfinite(Color.x) || !std::isfinite(Color.y) || !std::isfinite(Color.z) || !std::isfinite(Color.w)) {
            return false;
        }

        mShapeColor = Color;

        return true;
    }

    void Engine::UShapeComponent::SetDrawOnlyIfSelected(bool DrawOnlyIfSelected) {
        mDrawOnlyIfSelected = DrawOnlyIfSelected;
    }

}
