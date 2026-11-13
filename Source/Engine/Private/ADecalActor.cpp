/**
 * @file ADecalActor.cpp
 * @brief 데칼 컴포넌트를 루트로 소유하는 장면 액터입니다.
 * @ingroup Engine
 */

#include "Engine/ADecalActor.h"

#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/Component/UDecalComponent.h"

namespace Engine {

    bool Engine::ADecalActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::UDecalComponent>()};
        Component->SetName("DecalComponent");

        return AddComponent(std::move(Component));
    }

    Engine::UDecalComponent* Engine::ADecalActor::GetDecalComponent() const {
        return CoreUObject::Cast<Engine::UDecalComponent>(GetRootComponent());
    }

}
