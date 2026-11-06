/**
 * @file UCameraComponent.cpp
 * @brief 시야각 설정과 월드 변환으로부터 카메라 뷰 생성에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UCameraComponent.h"
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

    /**
     * @brief 카메라의 월드 변환과 시야각으로 뷰·투영 행렬을 계산합니다.
     */
    bool Engine::UCameraComponent::GetCameraView(const Core::FVector2& Size, Engine::FSceneView& View) const {
        if (!std::isfinite(Size.x) || !std::isfinite(Size.y) || Size.x <= 0.0f || Size.y <= 0.0f || !GetTransform().IsValid()) {
            return false;
        }

        const Core::FTransform& Transform{GetTransform()};
        Engine::FSceneView CameraView{};
        CameraView.mLocation = Transform.mLocation;
        CameraView.mForward = Transform.mRotation.ForwardVector();
        CameraView.mSize = Size;
        CameraView.mFieldOfView = mFieldOfView;
        CameraView.mView = Core::FMatrix::LookTo(CameraView.mLocation, CameraView.mForward, Transform.mRotation.UpVector());
        CameraView.mProjection = Core::FMatrix::Perspective(mFieldOfView, Size.x / Size.y, 0.05f, 10000.0f);
        CameraView.mViewProjection = CameraView.mView * CameraView.mProjection;
        View = CameraView;

        return true;
    }

}
