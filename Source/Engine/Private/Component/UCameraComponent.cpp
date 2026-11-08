/**
 * @file UCameraComponent.cpp
 * @brief 시야각 설정과 월드 변환으로부터 카메라 뷰 생성에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UCameraComponent.h"
#include "Engine/FMinimalViewInfo.h"
#include "Engine/FSceneView.h"
#include <cmath>

namespace Engine {

    /**
     * @brief 카메라의 수직 시야각을 도 단위로 반환합니다.
     */
    float Engine::UCameraComponent::GetFieldOfView() const {
        return mFieldOfView;
    }

    /**
     * @brief 유효한 범위의 시야각을 검사하여 카메라에 설정합니다.
     */
    bool Engine::UCameraComponent::SetFieldOfView(float FieldOfView) {
        if (!std::isfinite(FieldOfView) || FieldOfView <= 0.0f || FieldOfView >= 180.0f) {
            return false;
        }

        mFieldOfView = FieldOfView;

        return true;
    }

    Engine::FMinimalViewInfo Engine::UCameraComponent::GetCameraViewInfo() const {
        Engine::FMinimalViewInfo ViewInfo{};
        ViewInfo.mLocation = GetTransform().mLocation;
        ViewInfo.mRotation = GetTransform().mRotation;
        ViewInfo.mFieldOfView = mFieldOfView;

        return ViewInfo;
    }

    /**
     * @brief 카메라의 월드 변환과 시야각으로 뷰·투영 행렬을 계산합니다.
     */
    bool Engine::UCameraComponent::GetCameraView(const Core::FVector2& Size, Engine::FSceneView& View) const {
        return GetTransform().IsValid() && GetCameraViewInfo().CalcSceneView(Size, View);
    }

}
