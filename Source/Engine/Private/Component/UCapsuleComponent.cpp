/**
 * @file UCapsuleComponent.cpp
 * @brief Z축 중심 선분과 반구를 가진 캡슐 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UCapsuleComponent.h"
#include "Core/Archive.h"

#include <algorithm>

namespace Engine {

    /**
     * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
     */
    Engine::FCollisionShape Engine::UCapsuleComponent::GetCollisionShape() const {
        return Engine::FCollisionShape::MakeCapsule(GetScaledCapsuleRadius(), GetScaledCapsuleHalfHeight());
    }

    /**
     * @brief 충돌 형상의 로컬 축 정렬 경계를 계산합니다.
     */
    Core::FBox Engine::UCapsuleComponent::CalcLocalBounds() const {
        return Core::FBox{ Core::FVector{ -mCapsuleRadius, -mCapsuleRadius, -mCapsuleHalfHeight }, Core::FVector{ mCapsuleRadius, mCapsuleRadius, mCapsuleHalfHeight } };
    }

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
    void Engine::UCapsuleComponent::Serialize(Core::FArchive& Archive) {
        if (Archive.IsLoading() && !Archive.HasError() && Archive.HasField("CapsuleRadius")) {
            SetCapsuleRadius(0.0f);
        }

        Super::Serialize(Archive);
    }

    /**
     * @brief 캡슐의 로컬 반지름과 반높이를 함께 설정합니다.
     */
    void Engine::UCapsuleComponent::SetCapsuleSize(float InRadius, float InHalfHeight) {
        mCapsuleRadius = std::max(0.0f, InRadius);
        mCapsuleHalfHeight = std::max({ 0.0f, InHalfHeight, InRadius });
    }

    /**
     * @brief 월드 스케일을 반영한 캡슐 반지름을 반환합니다.
     */
    float Engine::UCapsuleComponent::GetScaledCapsuleRadius() const {
        return mCapsuleRadius * GetShapeScale();
    }

    /**
     * @brief 월드 스케일을 반영한 캡슐 반높이를 반환합니다.
     */
    float Engine::UCapsuleComponent::GetScaledCapsuleHalfHeight() const {
        return mCapsuleHalfHeight * GetShapeScale();
    }

    /**
     * @brief 반구를 제외한 캡슐 중심 선분 반길이를 반환합니다.
     */
    float Engine::UCapsuleComponent::GetScaledCapsuleHalfHeightWithoutHemisphere() const {
        return (mCapsuleHalfHeight - mCapsuleRadius) * GetShapeScale();
    }

    /**
     * @brief 반구를 포함한 캡슐 반높이를 반환합니다.
     */
    float Engine::UCapsuleComponent::GetCapsuleHalfHeight() const {
        return mCapsuleHalfHeight;
    }

    /**
     * @brief 반구를 포함한 캡슐 반높이를 설정합니다.
     */
    void Engine::UCapsuleComponent::SetCapsuleHalfHeight(float InHalfHeight) {
        mCapsuleHalfHeight = std::max(InHalfHeight, mCapsuleRadius);
    }

    /**
     * @brief 캡슐의 로컬 반지름을 반환합니다.
     */
    float Engine::UCapsuleComponent::GetCapsuleRadius() const {
        return mCapsuleRadius;
    }

    /**
     * @brief 캡슐의 로컬 반지름을 설정합니다.
     */
    void Engine::UCapsuleComponent::SetCapsuleRadius(float InRadius) {
        mCapsuleRadius = std::clamp(InRadius, 0.0f, mCapsuleHalfHeight);
    }

}
