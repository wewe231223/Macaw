/**
 * @file ASkeletalMeshActor.cpp
 * @brief 스켈레탈 메시 액터의 기본 컴포넌트를 구성합니다.
 * @ingroup Engine
 */
#include "Engine/ASkeletalMeshActor.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <utility>

namespace Engine {

    bool ASkeletalMeshActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        // 씬 복원 때와 같은 컴포넌트 구성을 사용하여 기본 루트가 중복되지 않게 합니다.
        auto Component{CoreUObject::NewObject<Engine::USkeletalMeshComponent>()};
        Component->SetName("SkeletalMeshComponent");
        return AddComponent(std::move(Component));
    }

    Engine::USkeletalMeshComponent* ASkeletalMeshActor::GetSkeletalMeshComponent() const {
        return CoreUObject::Cast<Engine::USkeletalMeshComponent>(GetRootComponent());
    }

}
