/**
 * @file UActorComponent.cpp
 * @brief 소유 액터와 월드 등록 수명을 갖는 컴포넌트의 기반 클래스입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UActorComponent.h"
#include "Engine/EngineStats.h"
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
    SetOuter(Owner);
    mOwner = Owner;
}

/**
 * @brief 컴포넌트를 월드에 연결하고 필요한 렌더 상태를 등록합니다.
 */
void UActorComponent::RegisterComponent(UWorld* World) {
    UWorld::FDispatchScope Scope{World};

    if (World == nullptr || mWorld == World || !World->CanAddActor() || (mOwner != nullptr && (mOwner->GetWorld() != World || mOwner->IsPendingDestroy()))) {
        return;
    }

    UnregisterComponent();

    if (!World->CanAddActor() || (mOwner != nullptr && (mOwner->GetWorld() != World || mOwner->IsPendingDestroy()))) {
        return;
    }

    mWorld = World;
    OnRegister();

    if (mOwner != nullptr && mOwner->HasBegunPlay() && World->CanBeginActor()) {
        DispatchBeginPlay();
    }
}

/**
 * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
 */
void UActorComponent::UnregisterComponent() {
    UWorld::FDispatchScope Scope{mWorld};
    DispatchEndPlay();
    mWorld = nullptr;
}

/**
 * @brief 컴포넌트가 월드에 등록되어 있는지 검사합니다.
 */
bool UActorComponent::IsRegistered() const {
    return mWorld != nullptr;
}

void UActorComponent::OnRegister() {
}

/**
 * @brief 컴포넌트의 게임 실행 시작 시 호출되는 확장 지점입니다.
 */
void UActorComponent::BeginPlay() {
}

/**
 * @brief 컴포넌트의 실행 또는 허용된 편집 갱신 시 호출되는 확장 지점입니다.
 */
void UActorComponent::Tick(float DeltaTime) {
}

/**
 * @brief 컴포넌트의 게임 실행 종료 시 호출되는 확장 지점입니다.
 */
void UActorComponent::EndPlay() {
}

/**
 * @brief 컴포넌트가 실행 시작 전달에 진입했는지 검사합니다.
 */
bool UActorComponent::HasBegunPlay() const {
    return mHasBegunPlay;
}

bool UActorComponent::IsEditorTickable() const {
    return mEditorTickable;
}

void UActorComponent::SetEditorTickable(bool EditorTickable) {
    mEditorTickable = EditorTickable;
}

/**
 * @brief 시작 상태를 설정하고 중복되지 않도록 시작 콜백을 전달합니다.
 */
void UActorComponent::DispatchBeginPlay() {
    if (!mHasBegunPlay && mWorld != nullptr && mOwner != nullptr && !mOwner->IsPendingDestroy()) {
        mHasBegunPlay = true;
        BeginPlay();
    }
}

/**
 * @brief 등록된 컴포넌트의 실행 상태 또는 편집 Tick 설정에 따라 갱신을 전달합니다.
 */
void UActorComponent::DispatchTick(float DeltaTime, bool EditorTick) {
    if (mWorld != nullptr && mOwner != nullptr && mOwner->GetWorld() == mWorld && !mOwner->IsPendingDestroy() && (EditorTick ? mEditorTickable : mHasBegunPlay)) {
        EngineStats::ComponentTicks.Add();
        Tick(DeltaTime);
    }
}

/**
 * @brief 시작 상태를 먼저 해제하여 종료 콜백의 중복 전달을 막습니다.
 */
void UActorComponent::DispatchEndPlay() {
    if (mHasBegunPlay) {
        mHasBegunPlay = false;
        EndPlay();
    }
}
