/**
 * @file AActor.cpp
 * @brief 컴포넌트를 소유하고 루트 컴포넌트를 통해 공간 변환을 제공하는 월드 객체입니다.
 * @ingroup Engine
 */

#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/UWorld.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/Reflection/Cast.h"

#include <utility>

/**
 * @brief AActor의 초기 상태와 전달받은 의존성을 구성합니다.
 */
AActor::AActor() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
AActor::~AActor() {
    SetWorld(nullptr);
}

/**
 * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
 */
UWorld* AActor::GetWorld() const {
    return mWorld;
}

/**
 * @brief 객체가 소속된 월드 연결을 지정합니다.
 */
void AActor::SetWorld(UWorld* World) {
    if (mWorld == World) {
        return;
    }

    for (const auto& Component : mComponents) {
        Component->UnregisterComponent();
    }

    mWorld = World;

    if (mWorld != nullptr) {
        for (const auto& Component : mComponents) {
            Component->RegisterComponent(*mWorld);
        }
    }
}

/**
 * @brief 컴포넌트의 소유권을 액터로 옮기고 필요하면 월드에 등록합니다.
 */
bool AActor::AddComponent(std::unique_ptr<UActorComponent> Component) {
    if (Component == nullptr || Component->GetOwner() != nullptr || Component->GetRegistry() != GetRegistry()) {
        return false;
    }

    Component->SetOwner(this);

    if (mRootComponent == nullptr) {
        mRootComponent = Cast<USceneComponent>(Component.get());
    }

    if (mWorld != nullptr) {
        Component->RegisterComponent(*mWorld);
    }

    mComponents.push_back(std::move(Component));

    return true;
}

/**
 * @brief 액터가 소유한 컴포넌트 목록을 반환합니다.
 */
const TArray<std::unique_ptr<UActorComponent>>& AActor::GetComponents() const {
    return mComponents;
}

/**
 * @brief 액터 변환을 제공하는 루트 컴포넌트의 비소유 주소를 반환합니다.
 */
USceneComponent* AActor::GetRootComponent() const {
    return mRootComponent;
}

/**
 * @brief 루트 공간 컴포넌트에 액터의 변환을 적용합니다.
 */
bool AActor::SetActorTransform(const FTransform& Transform) {
    return mRootComponent != nullptr && mRootComponent->SetTransform(Transform);
}

/**
 * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
 */
bool AActor::InitializeDefaultComponents() {
    return false;
}
