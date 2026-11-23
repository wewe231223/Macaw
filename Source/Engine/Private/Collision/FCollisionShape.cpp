/**
 * @file FCollisionShape.cpp
 * @brief 구·상자·캡슐의 로컬 치수를 담는 충돌 형상 값입니다.
 * @ingroup Engine
 */

#include "Core/Types.h"
#include "Engine/Collision/FCollisionShape.h"

#include <algorithm>

namespace Engine {

    /**
     * @brief 축별 반길이로 상자 충돌 형상을 만듭니다.
     */
    Engine::FCollisionShape Engine::FCollisionShape::MakeBox(const FVector& HalfExtent) {
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
     * @brief 상자의 로컬 축별 반길이를 반환합니다.
     */
    const FVector& Engine::FCollisionShape::GetBoxHalfExtent() const {
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

}
