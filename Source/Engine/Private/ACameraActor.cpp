/**
 * @file ACameraActor.cpp
 * @brief 카메라 컴포넌트를 갖고 이동·회전 입력을 처리하는 액터에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/ACameraActor.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/FViewportInput.h"
#include "Engine/UWorld.h"
#include "Core/Math/FRotator.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include <algorithm>
#include <cmath>
#include <utility>

bool ACameraActor::InitializeDefaultComponents() {
    if (!GetComponents().empty()) {
        return false;
    }
    auto Camera{NewObject<UCameraComponent>()};
    Camera->SetName("CameraComponent");
    return AddComponent(std::move(Camera));
}

UCameraComponent* ACameraActor::GetCameraComponent() const {
    return Cast<UCameraComponent>(GetRootComponent());
}

void ACameraActor::Input(const FViewportInput& Input, float DeltaTime) {
    UCameraComponent* Camera{GetCameraComponent()};
    if (Camera == nullptr || GetWorld() == nullptr || GetWorld()->GetPlayState() != EWorldPlayState::Playing || !HasBegunPlay() || IsPendingDestroy() || !Input.mFocused || !std::isfinite(DeltaTime) || DeltaTime < 0.0f || !std::isfinite(Input.mDelta.x) || !std::isfinite(Input.mDelta.y)) {
        return;
    }
    FTransform Transform{Camera->GetTransform()};
    FRotator Rotation{Transform.mRotation.Rotator()};
    if (Input.mRightDown) {
        Rotation.yaw += Input.mDelta.x * 0.2f;
        Rotation.pitch = std::clamp(Rotation.pitch - Input.mDelta.y * 0.2f, -89.0f, 89.0f);
        Transform.mRotation = Rotation.Quaternion();
    }
    const FVector Local{static_cast<float>(Input.mForward) - static_cast<float>(Input.mBackward), static_cast<float>(Input.mRight) - static_cast<float>(Input.mLeft), 0.0f};
    FVector Movement{Transform.mRotation * Local};
    Movement.z += static_cast<float>(Input.mUp) - static_cast<float>(Input.mDown);
    Transform.mLocation += Movement.Normalized() * DeltaTime * (Input.mFast ? 12.0f : 4.0f);
    Camera->SetTransform(Transform);
}
