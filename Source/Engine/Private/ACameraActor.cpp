/**
 * @file ACameraActor.cpp
 * @brief 카메라 컴포넌트를 갖고 이동·회전 입력을 처리하는 액터에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/ACameraActor.h"
#include "Engine/Component/UCameraComponent.h"
#include "Core/Input.h"
#include "Engine/UWorld.h"
#include "Core/Math/FRotator.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace Engine {

    bool Engine::ACameraActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Camera{CoreUObject::NewObject<Engine::UCameraComponent>()};
        Camera->SetName("CameraComponent");

        return AddComponent(std::move(Camera));
    }

    Engine::UCameraComponent* Engine::ACameraActor::GetCameraComponent() const {
        return CoreUObject::Cast<Engine::UCameraComponent>(GetRootComponent());
    }

    void Engine::ACameraActor::Input(const Core::FInput& Input, float DeltaTime) {
        using Core::EInputRoute;
        using Core::EKey;

        Engine::UCameraComponent* Camera{GetCameraComponent()};
        const Core::FVector2 MouseDelta{Input.GetMouseDelta(EInputRoute::Game)};

        if (Camera == nullptr || GetWorld() == nullptr || GetWorld()->GetPlayState() != Engine::EWorldPlayState::Playing || !HasBegunPlay() || IsPendingDestroy() || !Input.IsGameInputEnabled() || !std::isfinite(DeltaTime) || DeltaTime < 0.0f || !std::isfinite(MouseDelta.x) || !std::isfinite(MouseDelta.y)) {
            return;
        }

        Core::FTransform Transform{Camera->GetTransform()};
        Core::FRotator Rotation{Transform.mRotation.Rotator()};

        if (Input.IsDown(EKey::MouseRight, EInputRoute::Game)) {
            Rotation.yaw += MouseDelta.x * 0.2f;
            Rotation.pitch = std::clamp(Rotation.pitch - MouseDelta.y * 0.2f, -89.0f, 89.0f);
            Transform.mRotation = Rotation.Quaternion();
        }

        const Core::FVector Local{static_cast<float>(Input.IsDown(EKey::W, EInputRoute::Game)) - static_cast<float>(Input.IsDown(EKey::S, EInputRoute::Game)), static_cast<float>(Input.IsDown(EKey::D, EInputRoute::Game)) - static_cast<float>(Input.IsDown(EKey::A, EInputRoute::Game)), 0.0f};
        Core::FVector Movement{Transform.mRotation * Local};
        Movement.z += static_cast<float>(Input.IsDown(EKey::E, EInputRoute::Game)) - static_cast<float>(Input.IsDown(EKey::Q, EInputRoute::Game));
        Transform.mLocation += Movement.Normalized() * DeltaTime * (Input.IsDown(EKey::Shift, EInputRoute::Game) ? 12.0f : 4.0f);
        if (Input.IsDown(EKey::MouseRight, EInputRoute::Game) || Movement.LengthSquared() > 0.0f) {
            Camera->SetTransform(Transform);
        }
    }

}
