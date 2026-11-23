/**
 * @file FCollisionShape.h
 * @brief 구·상자·캡슐의 로컬 치수를 담는 충돌 형상 값입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "Core/Math/FVector.h"
#include "FCollisionShape.generated.h"

namespace Engine {

    /**
     * @brief 검사할 충돌 형상 종류입니다.
     */
    UENUM()
    enum class ECollisionShape : Uint8 {
        /**
         * @brief 충돌 형상을 사용하지 않습니다.
         */
        None,
        /**
         * @brief 축별 반길이를 갖는 상자 형상입니다.
         */
        Box,
        /**
         * @brief 반지름을 갖는 구 형상입니다.
         */
        Sphere,
        /**
         * @brief 선분 양 끝의 반구를 포함하는 캡슐 형상입니다.
         */
        Capsule
    };

    /**
     * @brief 구·상자·캡슐의 로컬 치수를 담는 충돌 형상 값입니다.
     */
    struct FCollisionShape {
    public:
        /**
         * @brief 축별 반길이로 상자 충돌 형상을 만듭니다.
         * @param[in] HalfExtent 상자의 로컬 축별 반길이입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        static Engine::FCollisionShape MakeBox(const FVector& HalfExtent);

        /**
         * @brief 반지름으로 구 충돌 형상을 만듭니다.
         * @param[in] Radius 형상 반지름입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        static Engine::FCollisionShape MakeSphere(float Radius);

        /**
         * @brief 반지름과 전체 반높이로 캡슐 충돌 형상을 만듭니다.
         * @param[in] Radius 형상 반지름입니다.
         * @param[in] HalfHeight 반구를 포함한 캡슐 반높이입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        static Engine::FCollisionShape MakeCapsule(float Radius, float HalfHeight);

        /**
         * @brief 상자 충돌 형상인지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsBox() const;

        /**
         * @brief 구 충돌 형상인지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsSphere() const;

        /**
         * @brief 캡슐 충돌 형상인지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsCapsule() const;

        /**
         * @brief 반구를 제외한 캡슐 중심 선분 반길이를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetCapsuleAxisHalfLength() const;

        /**
         * @brief 상자의 로컬 축별 반길이를 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const FVector& GetBoxHalfExtent() const;

        /**
         * @brief 충돌 형상의 반지름을 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetRadius() const;

        /**
         * @brief 반구를 포함한 캡슐 반높이를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetHalfHeight() const;

    private:
        /**
         * @brief 충돌 형상의 종류입니다.
         */
        Engine::ECollisionShape mShapeType{Engine::ECollisionShape::None};

        /**
         * @brief 상자의 로컬 축별 반길이입니다.
         */
        FVector mBoxHalfExtent{};

        /**
         * @brief 충돌 형상의 반지름입니다.
         */
        float mRadius{};

        /**
         * @brief 반구를 포함한 캡슐 반높이입니다.
         */
        float mHalfHeight{};
    };

}
