/**
 * @file AStaticMeshActor.cpp
 * @brief 기본 UStaticMeshComponent를 생성하는 프리미티브 액터입니다.
 * @ingroup Engine
 */

#include "Engine/AStaticMeshActor.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <utility>

/**
 * @brief AStaticMeshActor의 초기 상태와 전달받은 의존성을 구성합니다.
 */
AStaticMeshActor::AStaticMeshActor() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
AStaticMeshActor::~AStaticMeshActor() = default;

/**
 * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
 */
bool AStaticMeshActor::InitializeDefaultComponents() {
    if (!GetComponents().empty()) {
        return false;
    }

    auto Component{NewObject<UStaticMeshComponent>()};
    Component->SetName("StaticMeshComponent");

    return AddComponent(std::move(Component));
}

/**
 * @brief 액터의 기본 메시 컴포넌트 비소유 주소를 반환합니다.
 */
UStaticMeshComponent* AStaticMeshActor::GetStaticMeshComponent() const {
    return Cast<UStaticMeshComponent>(GetRootComponent());
}
