/**
 * @file USceneComponent.cpp
 * @brief 검증된 위치, 회전과 스케일을 보관하는 공간 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/USceneComponent.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FTransform.h"

namespace Engine {

    /**
     * @brief USceneComponent의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Engine::USceneComponent::USceneComponent() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::USceneComponent::~USceneComponent() = default;

    /**
     * @brief 컴포넌트의 위치·회전·스케일을 반환합니다.
     */
    const Core::FTransform& Engine::USceneComponent::GetTransform() const {
        return mTransform;
    }

    /**
     * @brief 유한하고 유효한 변환을 검증하여 공간 컴포넌트에 적용합니다.
     */
    bool Engine::USceneComponent::SetTransform(const Core::FTransform& Transform) {
        if (!Transform.IsValid()) {
            return false;
        }

        mTransform = Transform;

        return true;
    }

    /**
     * @brief 위치, 회전과 스케일이 적용된 월드 행렬을 반환합니다.
     */
    Core::FMatrix Engine::USceneComponent::GetWorldMatrix() const {
        return mTransform.ToMatrix();
    }

}
