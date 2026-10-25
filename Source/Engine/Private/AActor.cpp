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
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"

#include <algorithm>
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
    if (mWorld == World || mChangingWorld) {
        return;
    }

    UWorld::FDispatchScope Scope{mWorld};
    mChangingWorld = true;
    DispatchEndPlay();

    for (const auto& Component : mComponents) {
        Component->UnregisterComponent();
    }

    mWorld = World;

    if (mWorld != nullptr) {
        for (const auto& Component : mComponents) {
            Component->RegisterComponent(mWorld);
        }
    }

    mChangingWorld = false;
}

/**
 * @brief 컴포넌트의 소유권을 액터로 옮기고 필요하면 월드에 등록합니다.
 */
bool AActor::AddComponent(std::unique_ptr<UActorComponent> Component) {
    if (Component == nullptr || Component->GetOwner() != nullptr || Component->GetWorld() != nullptr || mPendingDestroy || mChangingWorld || (mWorld != nullptr && !mWorld->CanAddActor())) {
        return false;
    }

    UWorld::FDispatchScope Scope{mWorld};
    UActorComponent* Added{Component.get()};
    Added->SetOwner(this);

    if (mRootComponent == nullptr) {
        mRootComponent = Cast<USceneComponent>(Added);
    }

    mComponents.push_back(std::move(Component));

    if (mWorld != nullptr) {
        Added->RegisterComponent(mWorld);

        if (mHasBegunPlay && mWorld->CanBeginActor()) {
            Added->DispatchBeginPlay();
        }
    }

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
 * @brief 소유한 공간 컴포넌트를 액터의 루트로 지정합니다.
 */
bool AActor::SetRootComponent(USceneComponent* Component) {
    if (Component != nullptr && std::none_of(mComponents.begin(), mComponents.end(), [Component](const auto& Entry) {
        return Entry.get() == Component;
    })) {
        return false;
    }

    mRootComponent = Component;
    return true;
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
    return true;
}

/**
 * @brief 액터의 게임 실행 시작 시 호출되는 확장 지점입니다.
 */
void AActor::BeginPlay() {
}

/**
 * @brief 액터의 실행 또는 허용된 편집 갱신 시 호출되는 확장 지점입니다.
 */
void AActor::Tick(float DeltaTime) {
}

/**
 * @brief 액터의 게임 실행 종료 시 호출되는 확장 지점입니다.
 */
void AActor::EndPlay() {
}

void AActor::Input(const FViewportInput&, float) {
}

/**
 * @brief 액터가 실행 시작 전달에 진입했는지 검사합니다.
 */
bool AActor::HasBegunPlay() const {
    return mHasBegunPlay;
}

/**
 * @brief 지연 삭제가 예약되었는지 검사합니다.
 */
bool AActor::IsPendingDestroy() const {
    return mPendingDestroy;
}

bool AActor::IsEditorTickable() const {
    return mEditorTickable;
}

void AActor::SetEditorTickable(bool EditorTickable) {
    mEditorTickable = EditorTickable;
}

/**
 * @brief 콜백 중 컴포넌트 추가에 영향받지 않는 핸들 스냅샷을 만듭니다.
 */
TArray<FObjectHandle> AActor::GetComponentHandles() const {
    TArray<FObjectHandle> Handles{};
    Handles.reserve(mComponents.size());

    for (const auto& Component : mComponents) {
        Handles.push_back(Component->GetHandle());
    }

    return Handles;
}

/**
 * @brief 컴포넌트를 먼저 시작한 뒤 살아 있는 액터의 시작 콜백을 전달합니다.
 */
void AActor::DispatchBeginPlay() {
    if (mHasBegunPlay || mPendingDestroy || mWorld == nullptr || !mWorld->CanBeginActor()) {
        return;
    }

    mHasBegunPlay = true;
    const auto Handles{GetComponentHandles()};

    for (const FObjectHandle Handle : Handles) {
        if (mPendingDestroy || !mWorld->CanBeginActor()) {
            break;
        }

        if (auto* Component{Cast<UActorComponent>(FObjectRegistry::Get()->Resolve(Handle))}; Component != nullptr && Component->GetOwner() == this) {
            Component->DispatchBeginPlay();
        }
    }

    if (!mPendingDestroy && mWorld->CanBeginActor()) {
        mBeginPlayCalled = true;
        BeginPlay();
    }
}

/**
 * @brief 액터와 각 컴포넌트의 실행 상태 및 편집 Tick 설정에 따라 갱신을 전달합니다.
 */
void AActor::DispatchTick(float DeltaTime, bool EditorTick) {
    if (mWorld == nullptr || mPendingDestroy || (!EditorTick && !mHasBegunPlay)) {
        return;
    }

    const auto Handles{GetComponentHandles()};
    if (!EditorTick || mEditorTickable) {
        Tick(DeltaTime);
    }

    for (const FObjectHandle Handle : Handles) {
        if (mPendingDestroy || mWorld == nullptr || (!EditorTick && !mHasBegunPlay) || mWorld->mEndPlayRequested || mWorld->mShutdownRequested) {
            break;
        }

        if (auto* Component{Cast<UActorComponent>(FObjectRegistry::Get()->Resolve(Handle))}; Component != nullptr && Component->GetOwner() == this) {
            Component->DispatchTick(DeltaTime, EditorTick);
        }
    }
}

/**
 * @brief 시작된 액터의 종료 콜백과 컴포넌트 역순 종료를 전달합니다.
 */
void AActor::DispatchEndPlay() {
    if (!mHasBegunPlay) {
        return;
    }

    mHasBegunPlay = false;
    const auto Handles{GetComponentHandles()};

    if (mBeginPlayCalled) {
        mBeginPlayCalled = false;
        EndPlay();
    }

    for (auto Position{Handles.rbegin()}; Position != Handles.rend(); ++Position) {
        if (auto* Component{Cast<UActorComponent>(FObjectRegistry::Get()->Resolve(*Position))}; Component != nullptr && Component->GetOwner() == this) {
            Component->DispatchEndPlay();
        }
    }
}
