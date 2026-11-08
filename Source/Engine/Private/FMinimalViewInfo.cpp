#include "Engine/FMinimalViewInfo.h"
#include "Engine/FSceneView.h"
#include "Core/Math/FMatrix.h"
#include <cmath>

namespace Engine {

    Engine::FMinimalViewInfo Engine::FMinimalViewInfo::Blend(const Engine::FMinimalViewInfo& Other, float Alpha) const {
        Engine::FMinimalViewInfo Result{};
        Result.mLocation = mLocation.Lerp(Other.mLocation, Alpha);
        Result.mRotation = mRotation.Slerp(Other.mRotation, Alpha);
        Result.mFieldOfView = mFieldOfView + (Other.mFieldOfView - mFieldOfView) * Alpha;

        return Result;
    }

    bool Engine::FMinimalViewInfo::CalcSceneView(const Core::FVector2& Size, Engine::FSceneView& View) const {
        if (!std::isfinite(Size.x) || !std::isfinite(Size.y) || Size.x <= 0.0f || Size.y <= 0.0f || !mLocation.IsFinite() || !mRotation.IsFinite() || !std::isfinite(mFieldOfView) || mFieldOfView <= 0.0f || mFieldOfView >= 180.0f) {
            return false;
        }

        Engine::FSceneView CameraView{};
        CameraView.mLocation = mLocation;
        CameraView.mForward = mRotation.ForwardVector();
        CameraView.mSize = Size;
        CameraView.mFieldOfView = mFieldOfView;
        CameraView.mView = Core::FMatrix::LookTo(CameraView.mLocation, CameraView.mForward, mRotation.UpVector());
        CameraView.mProjection = Core::FMatrix::Perspective(mFieldOfView, Size.x / Size.y, 0.05f, 10000.0f);
        CameraView.mViewProjection = CameraView.mView * CameraView.mProjection;
        View = CameraView;

        return true;
    }

}
