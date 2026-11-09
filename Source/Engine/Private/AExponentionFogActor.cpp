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

    /**
     * @brief 기본 설정으로 AExponentionFogActor 객체를 초기화합니다.
     */
    Engine::AExponentionFogActor::AExponentionFogActor() = default;

    /**
     * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Engine::AExponentionFogActor::~AExponentionFogActor() = default;

    /**
     * @brief 빈 액터에 HeightFogComponent를 생성하고 루트 컴포넌트로 추가합니다.
     */
    bool Engine::AExponentionFogActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::UHeightFogComponent>()};
        Component->SetName("HeightFogComponent");

        return AddComponent(std::move(Component));
    }

    /**
     * @brief 안개 액터의 루트에 연결된 높이 안개 컴포넌트를 조회합니다.
     */
    Engine::UHeightFogComponent* Engine::AExponentionFogActor::GetFogComponent() const {
        return CoreUObject::Cast<Engine::UHeightFogComponent>(GetRootComponent());
    }

}
