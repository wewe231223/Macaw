/**
 * @file AExponentionFogActor.cpp
 * @brief 높이 기반 지수 안개 컴포넌트를 루트로 생성하여 에디터에서 배치하는 액터입니다.
 * @ingroup Engine
 */

#include "Engine/AExponentionFogActor.h"
#include "Engine/Component/UHeightFogComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

namespace Engine {

    Engine::AExponentionFogActor::AExponentionFogActor() = default;
    Engine::AExponentionFogActor::~AExponentionFogActor() = default;

    bool Engine::AExponentionFogActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::UHeightFogComponent>()};
        Component->SetName("HeightFogComponent");

        return AddComponent(std::move(Component));
    }

    Engine::UHeightFogComponent* Engine::AExponentionFogActor::GetFogComponent() const {
        return CoreUObject::Cast<Engine::UHeightFogComponent>(GetRootComponent());
    }

}
