/**
 * @file AActor.cpp
 * @brief 컴포넌트를 소유하고 루트 컴포넌트를 통해 공간 변환을 제공하는 월드 객체입니다.
 * @ingroup Engine
 */

#include "Engine/AActor.h"
#include "Engine/EngineStats.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/UWorld.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"

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
     * @brief 객체가 소속된 월드 연결을 지정합니다.
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
    bool Engine::AActor::SetActorTransform(const Core::FTransform& Transform) {
        if (mRootComponent == nullptr) {
            return false;
        }

        const Core::FTransform Previous{mRootComponent->GetTransform()};

        if (!mRootComponent->SetTransform(Transform)) {
            return false;
        }

        // 컴포넌트 변환은 월드 기준이고 계층이 없으므로, 충돌 도형 같은 나머지 컴포넌트를 루트와 함께 옮깁니다.
        const Core::FQuat Inverse{Previous.mRotation.Inverse()};
        const Core::FQuat Rotation{Transform.mRotation * Inverse};
        const auto ScaleRatio{[](float New, float Old) {
            return Old != 0.0f ? New / Old : 1.0f;
        }};

        const Core::FVector Ratio{ScaleRatio(Transform.mScale.x, Previous.mScale.x), ScaleRatio(Transform.mScale.y, Previous.mScale.y), ScaleRatio(Transform.mScale.z, Previous.mScale.z)};

        for (const auto& Component : mComponents) {
            auto* Scene{CoreUObject::Cast<Engine::USceneComponent>(Component.get())};

            if (Scene == nullptr || Scene == mRootComponent) {
                continue;
            }

            Core::FTransform Moved{Scene->GetTransform()};
            const Core::FVector Local{Inverse.RotateVector(Moved.mLocation - Previous.mLocation)};

            Moved.mLocation = Transform.mLocation + Transform.mRotation.RotateVector(Core::FVector{Local.x * Ratio.x, Local.y * Ratio.y, Local.z * Ratio.z});
            Moved.mRotation = Rotation * Moved.mRotation;
            Moved.mScale = Core::FVector{Moved.mScale.x * Ratio.x, Moved.mScale.y * Ratio.y, Moved.mScale.z * Ratio.z};

            Scene->SetTransform(Moved);
        }

        return true;
    }

    /**
     * @brief 루트 컴포넌트의 변환을 반환합니다.
     */
    Core::FTransform Engine::AActor::GetActorTransform() const {
        return mRootComponent != nullptr ? mRootComponent->GetTransform() : Core::FTransform{};
    }

    /**
     * @brief 월드에서 이 액터를 제거합니다. 실행 전달 중이면 안전한 시점까지 미룹니다.
     */
    bool Engine::AActor::Destroy() {
        return mWorld != nullptr && mWorld->DestroyActor(GetHandle());
    }

    /**
     * @brief 지정한 클래스의 첫 소유 컴포넌트를 찾습니다.
     */
    Engine::UActorComponent* Engine::AActor::FindComponentByClassName(const Core::FString& ClassName) const {
        const CoreUObject::UClass* Class{CoreUObject::FReflectionRegistry::FindClass(ClassName)};

        for (const auto& Component : mComponents) {
            if (Class != nullptr && Component->IsA(Class)) {
                return Component.get();
            }
        }

        return nullptr;
    }

    /**
     * @brief 지정한 클래스의 소유 컴포넌트를 모두 찾습니다.
     */
    Core::TArray<Engine::UActorComponent*> Engine::AActor::GetComponentsByClassName(const Core::FString& ClassName) const {
        const CoreUObject::UClass* Class{CoreUObject::FReflectionRegistry::FindClass(ClassName)};
        Core::TArray<Engine::UActorComponent*> Components{};

        for (const auto& Component : mComponents) {
            if (Class != nullptr && Component->IsA(Class)) {
                Components.push_back(Component.get());
            }
        }

        return Components;
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
     * @brief 액터의 실행 또는 허용된 편집 갱신 시 호출되는 확장 지점입니다.
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
    Core::TArray<CoreUObject::FObjectHandle> Engine::AActor::GetComponentHandles() const {
        Core::TArray<CoreUObject::FObjectHandle> Handles{};
        Handles.reserve(mComponents.size());

        for (const auto& Component : mComponents) {
            Handles.push_back(Component->GetHandle());
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

        const auto Handles{GetComponentHandles()};

        for (const CoreUObject::FObjectHandle Handle : Handles) {
            if (mPendingDestroy || !mWorld->CanBeginActor()) {
                break;
            }

            if (auto* Component{CoreUObject::Cast<Engine::UActorComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(Handle))}; Component != nullptr && Component->GetOwner() == this) {
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
    void Engine::AActor::DispatchTick(float DeltaTime, bool EditorTick) {
        if (mWorld == nullptr || mPendingDestroy || (!EditorTick && !mHasBegunPlay)) {
            return;
        }

        const auto Handles{GetComponentHandles()};

        if (!EditorTick || mEditorTickable) {
            Engine::EngineStats::ActorTicks.Add();
            Tick(DeltaTime);
        }

        for (const CoreUObject::FObjectHandle Handle : Handles) {
            if (mPendingDestroy || mWorld == nullptr || (!EditorTick && !mHasBegunPlay) || mWorld->mEndPlayRequested || mWorld->mShutdownRequested) {
                break;
            }

            if (auto* Component{CoreUObject::Cast<Engine::UActorComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(Handle))}; Component != nullptr && Component->GetOwner() == this) {
                Component->DispatchTick(DeltaTime, EditorTick);
            }
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

        const auto Handles{GetComponentHandles()};

        if (mBeginPlayCalled) {
            mBeginPlayCalled = false;

            EndPlay();
        }

        for (auto Position{Handles.rbegin()}; Position != Handles.rend(); ++Position) {
            if (auto* Component{CoreUObject::Cast<Engine::UActorComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(*Position))}; Component != nullptr && Component->GetOwner() == this) {
                Component->DispatchEndPlay();
            }
        }
    }

}
