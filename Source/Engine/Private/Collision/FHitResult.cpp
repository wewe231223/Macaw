/**
 * @file FHitResult.cpp
 * @brief 충돌 이벤트의 접촉 정보와 비소유 객체 참조입니다.
 * @ingroup Engine
 */

#include "Engine/Collision/FHitResult.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/AActor.h"
#include "Engine/Component/UPrimitiveComponent.h"

namespace Engine {

    /**
     * @brief 적중한 액터의 비소유 주소를 반환합니다.
     */
    Engine::AActor* Engine::FHitResult::GetActor() const {
        Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(mHitActor.Get())};

        return Actor != nullptr && !Actor->IsPendingDestroy() ? Actor : nullptr;
    }

    /**
     * @brief 적중한 프리미티브 컴포넌트의 비소유 주소를 반환합니다.
     */
    Engine::UPrimitiveComponent* Engine::FHitResult::GetComponent() const {
        Engine::UPrimitiveComponent* Component{CoreUObject::Cast<Engine::UPrimitiveComponent>(mHitComponent.Get())};
        const Engine::AActor* Owner{Component != nullptr ? Component->GetOwner() : nullptr};

        return Owner == nullptr || !Owner->IsPendingDestroy() ? Component : nullptr;
    }

    void Engine::FHitResult::SetActor(Engine::AActor* Actor) {
        mHitActor = Actor;
    }

    void Engine::FHitResult::SetComponent(Engine::UPrimitiveComponent* Component) {
        mHitComponent = Component;
    }

}
