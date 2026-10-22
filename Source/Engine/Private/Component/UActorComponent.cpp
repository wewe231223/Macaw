/**
 * @file UActorComponent.cpp
 * @brief 소유 액터와 월드 등록 수명을 갖는 컴포넌트의 기반 클래스입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UActorComponent.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"

/**
 * @brief UActorComponent의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UActorComponent::UActorComponent() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UActorComponent::~UActorComponent() = default;

/**
 * @brief 소유 객체 또는 메타데이터를 반환합니다.
 */
AActor* UActorComponent::GetOwner() const {
    return mOwner;
}

/**
 * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
 */
UWorld* UActorComponent::GetWorld() const {
    return mWorld;
}

/**
 * @brief 컴포넌트의 소유 액터를 지정합니다.
 */
void UActorComponent::SetOwner(AActor* Owner) {
    mOwner = Owner;
}

/**
 * @brief 컴포넌트를 월드에 연결하고 필요한 렌더 상태를 등록합니다.
 */
void UActorComponent::RegisterComponent(UWorld& World) {
    mWorld = &World;
}

/**
 * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
 */
void UActorComponent::UnregisterComponent() {
    mWorld = nullptr;
}

/**
 * @brief 컴포넌트가 월드에 등록되어 있는지 검사합니다.
 */
bool UActorComponent::IsRegistered() const {
    return mWorld != nullptr;
}
