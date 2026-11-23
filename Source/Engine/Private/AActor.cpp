/**
 * @file AActor.cpp
 * @brief 컴포넌트를 소유하고 루트 컴포넌트를 통해 공간 변환을 제공하는 월드 객체입니다.
 * @ingroup Engine
 */

#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"

#include <algorithm>
#include <utility>

namespace Engine {

    /**
     * @brief AActor의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Engine::AActor::AActor() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::AActor::~AActor() {
        SetWorld(nullptr);
    }

    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     */
    Engine::UWorld* Engine::AActor::GetWorld() const {
        return mWorld;
    }

    /**
     * @brief 액터의 직접 외부 객체에서 Tick을 등록할 레벨을 찾습니다.
     */
    Engine::ULevel* Engine::AActor::GetLevel() const {
        return CoreUObject::Cast<Engine::ULevel>(GetOuter());
    }

    /**
     * @brief 객체가 소속된 월드 연결을 지정합니다.
     * @details 기존 컴포넌트와 액터 Tick 등록을 해제한 뒤 새 월드에서 액터 Tick을 먼저 등록합니다.
     */
    void Engine::AActor::SetWorld(Engine::UWorld* World) {
        if (mWorld == World || mChangingWorld) {
            return;
        }

        Engine::UWorld::FDispatchScope Scope{mWorld};

        mChangingWorld = true;

        DispatchEndPlay();

        for (const auto& Component : mComponents) {
            Component->UnregisterComponent();
        }

        if (mWorld != nullptr) {
            RegisterActorTickFunctions(false);
        }

        mWorld = World;

        if (mWorld != nullptr) {
            RegisterActorTickFunctions(true);

            for (const auto& Component : mComponents) {
                Component->RegisterComponent(mWorld);
            }
        }

        mChangingWorld = false;
    }

    /**
     * @brief 컴포넌트의 소유권을 액터로 옮기고 필요하면 월드에 등록합니다.
     */
    bool Engine::AActor::AddComponent(std::unique_ptr<Engine::UActorComponent> Component) {
        if (Component == nullptr || Component->GetOwner() != nullptr || Component->GetWorld() != nullptr || mPendingDestroy || mChangingWorld || (mWorld != nullptr && !mWorld->CanAddActor())) {
            return false;
        }

        Engine::UWorld::FDispatchScope Scope{mWorld};
        Engine::UActorComponent* Added{Component.get()};
        Added->SetOwner(this);

        if (mRootComponent == nullptr) {
            mRootComponent = CoreUObject::Cast<Engine::USceneComponent>(Added);
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
    const Core::TArray<std::unique_ptr<Engine::UActorComponent>>& Engine::AActor::GetComponents() const {
        return mComponents;
    }

    Engine::UActorComponent* Engine::AActor::FindComponentByClass(const CoreUObject::UClass* Class) const {
        for (const auto& Component : mComponents) {
            if (Component->IsA(Class)) {
                return Component.get();
            }
        }

        return nullptr;
    }

    Engine::AActor::FComponentCollisionDelegate& Engine::AActor::GetOnComponentCollision() {
        return mOnComponentCollision;
    }

    /**
     * @brief 액터 변환을 제공하는 루트 컴포넌트의 비소유 주소를 반환합니다.
     */
    Engine::USceneComponent* Engine::AActor::GetRootComponent() const {
        return mRootComponent;
    }

    /**
     * @brief 소유한 공간 컴포넌트를 액터의 루트로 지정합니다.
     */
    bool Engine::AActor::SetRootComponent(Engine::USceneComponent* Component) {
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
    bool Engine::AActor::SetActorTransform(const FTransform& Transform) {
        if (mRootComponent == nullptr) {
            return false;
        }

        const FTransform Previous{mRootComponent->GetTransform()};

        if (!mRootComponent->SetTransform(Transform)) {
            return false;
        }

        // 컴포넌트 변환은 월드 기준이고 계층이 없으므로, 충돌 도형 같은 나머지 컴포넌트를 루트와 함께 옮깁니다.
        const FQuat Inverse{Previous.mRotation.Inverse()};
        const FQuat Rotation{Transform.mRotation * Inverse};
        const auto ScaleRatio{[](float New, float Old) {
            return Old != 0.0f ? New / Old : 1.0f;
        }};

        const FVector Ratio{ScaleRatio(Transform.mScale.x, Previous.mScale.x), ScaleRatio(Transform.mScale.y, Previous.mScale.y), ScaleRatio(Transform.mScale.z, Previous.mScale.z)};

        for (const auto& Component : mComponents) {
            auto* Scene{CoreUObject::Cast<Engine::USceneComponent>(Component.get())};

            if (Scene == nullptr || Scene == mRootComponent) {
                continue;
            }

            FTransform Moved{Scene->GetTransform()};
            const FVector Local{Inverse.RotateVector(Moved.mLocation - Previous.mLocation)};

            Moved.mLocation = Transform.mLocation + Transform.mRotation.RotateVector(FVector{Local.x * Ratio.x, Local.y * Ratio.y, Local.z * Ratio.z});
            Moved.mRotation = Rotation * Moved.mRotation;
            Moved.mScale = FVector{Moved.mScale.x * Ratio.x, Moved.mScale.y * Ratio.y, Moved.mScale.z * Ratio.z};

            Scene->SetTransform(Moved);
        }

        return true;
    }

    /**
     * @brief 루트 컴포넌트의 변환을 반환합니다.
     */
    FTransform Engine::AActor::GetActorTransform() const {
        return mRootComponent != nullptr ? mRootComponent->GetTransform() : FTransform{};
    }

    /**
     * @brief 월드에서 이 액터를 제거합니다. 실행 전달 중이면 안전한 시점까지 미룹니다.
     */
    bool Engine::AActor::Destroy() {
        return mWorld != nullptr && mWorld->DestroyActor(CoreUObject::MakeObjectRef(this));
    }

    /**
     * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
     */
    bool Engine::AActor::InitializeDefaultComponents() {
        return true;
    }

    /**
     * @brief 액터의 게임 실행 시작 시 호출되는 확장 지점입니다.
     */
    void Engine::AActor::BeginPlay() {
    }

    /**
     * @brief 기본 액터 Tick 함수가 실행 차례에 호출하는 갱신 확장 지점입니다.
     */
    void Engine::AActor::Tick(float DeltaTime) {
    }

    /**
     * @brief 액터의 게임 실행 종료 시 호출되는 확장 지점입니다.
     */
    void Engine::AActor::EndPlay() {
    }

    /**
     * @brief 뷰포트 입력을 받아 프레임 시간에 따라 액터의 동작을 갱신합니다.
     */
    void Engine::AActor::Input(const Core::FInput&, float) {
    }

    /**
     * @brief 액터가 실행 시작 전달에 진입했는지 검사합니다.
     */
    bool Engine::AActor::HasBegunPlay() const {
        return mHasBegunPlay;
    }

    /**
     * @brief 지연 삭제가 예약되었는지 검사합니다.
     */
    bool Engine::AActor::IsPendingDestroy() const {
        return mPendingDestroy;
    }

    /**
     * @brief 정지한 편집 월드에서도 Tick을 허용하는지 반환합니다.
     */
    bool Engine::AActor::IsEditorTickable() const {
        return mEditorTickable;
    }

    /**
     * @brief 정지한 편집 월드에서의 Tick 허용 여부를 설정합니다.
     */
    void Engine::AActor::SetEditorTickable(bool EditorTickable) {
        mEditorTickable = EditorTickable;
    }

    /**
     * @brief 액터가 직접 소유한 기본 Tick 함수를 반환합니다.
     */
    Engine::FActorTickFunction& Engine::AActor::GetPrimaryActorTick() {
        return mPrimaryActorTick;
    }

    /**
     * @brief 기본 액터 Tick에 지정한 그룹을 조회합니다.
     */
    Engine::ETickingGroup Engine::AActor::GetTickGroup() const {
        return mPrimaryActorTick.GetTickGroup();
    }

    /**
     * @brief 검증과 그룹 설정을 기본 액터 Tick에 전달합니다.
     */
    bool Engine::AActor::SetTickGroup(Engine::ETickingGroup Group) {
        return mPrimaryActorTick.SetTickGroup(Group);
    }

    /**
     * @brief 등록을 요청하면 소속 레벨에서 기본 Tick을 등록하고, 해제 시에는 기존 등록을 끊습니다.
     */
    void Engine::AActor::RegisterActorTickFunctions(bool Register) {
        if (!Register) {
            mPrimaryActorTick.UnregisterTickFunction();
            return;
        }

        if (Engine::ULevel* Level{GetLevel()}) {
            mPrimaryActorTick.RegisterTickFunction(*Level);
        }
    }

    /**
     * @brief 액터의 표시 여부를 반환합니다.
     */
    bool Engine::AActor::IsVisible() const {
        return mVisible;
    }

    /**
     * @brief 액터의 표시 여부를 변경하고 소유 컴포넌트의 렌더 상태에 반영합니다.
     */
    void Engine::AActor::SetVisible(bool Visible) {
        if (mVisible == Visible) {
            return;
        }

        mVisible = Visible;

        for (const auto& Component : mComponents) {
            Component->OnOwnerVisibilityChanged();
        }
    }

    /**
     * @brief 콜백 중 컴포넌트 추가에 영향받지 않는 핸들 스냅샷을 만듭니다.
     */
    Core::TArray<CoreUObject::TObjectRef<Engine::UActorComponent>> Engine::AActor::GetComponentRefs() const {
        Core::TArray<CoreUObject::TObjectRef<Engine::UActorComponent>> Handles{};
        Handles.reserve(mComponents.size());

        for (const auto& Component : mComponents) {
            Handles.push_back(CoreUObject::MakeObjectRef(Component));
        }

        return Handles;
    }

    /**
     * @brief 컴포넌트를 먼저 시작한 뒤 살아 있는 액터의 시작 콜백을 전달합니다.
     */
    void Engine::AActor::DispatchBeginPlay() {
        if (mHasBegunPlay || mPendingDestroy || mWorld == nullptr || !mWorld->CanBeginActor()) {
            return;
        }

        mHasBegunPlay = true;

        const auto Handles{GetComponentRefs()};

        for (const CoreUObject::TObjectRef<Engine::UActorComponent> Handle : Handles) {
            if (mPendingDestroy || !mWorld->CanBeginActor()) {
                break;
            }

            if (auto* Component{CoreUObject::Cast<Engine::UActorComponent>(Handle.Get())}; Component != nullptr && Component->GetOwner() == this) {
                Component->DispatchBeginPlay();
            }
        }

        if (!mPendingDestroy && mWorld->CanBeginActor()) {
            mBeginPlayCalled = true;

            BeginPlay();
        }
    }

    /**
     * @brief 시작된 액터의 종료 콜백과 컴포넌트 역순 종료를 전달합니다.
     */
    void Engine::AActor::DispatchEndPlay() {
        if (!mHasBegunPlay) {
            return;
        }

        mHasBegunPlay = false;

        const auto Handles{GetComponentRefs()};

        if (mBeginPlayCalled) {
            mBeginPlayCalled = false;

            EndPlay();
        }

        for (auto Position{Handles.rbegin()}; Position != Handles.rend(); ++Position) {
            if (auto* Component{CoreUObject::Cast<Engine::UActorComponent>((*Position).Get())}; Component != nullptr && Component->GetOwner() == this) {
                Component->DispatchEndPlay();
            }
        }
    }

}
