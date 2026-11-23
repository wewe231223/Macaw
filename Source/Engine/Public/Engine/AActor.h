/**
 * @file AActor.h
 * @brief 컴포넌트를 소유하고 루트 컴포넌트를 통해 공간 변환을 제공하는 월드 객체입니다.
 * @ingroup Engine
 */

#pragma once

#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/TDelegate.h"

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"
#include "Engine/Tick/FTickFunction.h"
#include "Core/Input.h"
#include "Core/Math/FTransform.h"
#include "Engine/Collision/FHitResult.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "AActor.generated.h"

namespace Engine {

    /**
     * @brief 컴포넌트를 소유하고 루트 컴포넌트를 통해 공간 변환을 제공하는 월드 객체입니다.
     * @ingroup Engine
     */
    UCLASS()
    class AActor : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        enum class EComponentCollision {
            BeginOverlap,
            EndOverlap,
            Hit
        };

        using FComponentCollisionDelegate = CoreUObject::TDelegate<Engine::AActor::EComponentCollision, Engine::UPrimitiveComponent*, Engine::AActor*, Engine::UPrimitiveComponent*, const Engine::FHitResult*>;

    public:
        /**
         * @brief AActor의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        AActor();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~AActor() override;

    public:
        /**
         * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::UWorld* GetWorld() const;

        /**
         * @brief 액터의 직접 외부 객체가 레벨이면 해당 레벨을 반환합니다.
         * @return 레벨의 비소유 주소이며, 레벨 밖의 액터는 nullptr입니다.
         */
        Engine::ULevel* GetLevel() const;

        /**
         * @brief 컴포넌트의 소유권을 액터로 옮기고 필요하면 월드에 등록합니다.
         * @param[in] Component 액터가 소유권을 넘겨받을 컴포넌트입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool AddComponent(std::unique_ptr<Engine::UActorComponent> Component);

        /**
         * @brief 액터가 소유한 컴포넌트 목록을 반환합니다.
         * @return 소유 목록의 읽기 전용 참조입니다.
         */
        const Core::TArray<std::unique_ptr<Engine::UActorComponent>>& GetComponents() const;
        Engine::UActorComponent* FindComponentByClass(const CoreUObject::UClass* Class) const;

        FComponentCollisionDelegate& GetOnComponentCollision();

        /**
         * @brief 액터 변환을 제공하는 루트 컴포넌트의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::USceneComponent* GetRootComponent() const;

        /**
         * @brief 소유한 공간 컴포넌트를 액터의 루트로 지정합니다.
         * @param[in] Component 이 액터가 소유한 공간 컴포넌트이며 nullptr이면 루트를 해제합니다.
         * @return 소유 관계를 검증하여 지정하면 true입니다.
         */
        bool SetRootComponent(Engine::USceneComponent* Component);

        /**
         * @brief 루트 공간 컴포넌트에 액터의 변환을 적용합니다.
         * @param[in] Transform 적용할 위치, 회전과 스케일입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool SetActorTransform(const FTransform& Transform);

        /**
         * @brief 루트 컴포넌트의 변환을 반환합니다.
         * @return 루트 컴포넌트의 변환이며, 루트가 없으면 기본 변환입니다.
         */
        FTransform GetActorTransform() const;

        /**
         * @brief 월드에서 이 액터를 제거합니다. 실행 전달 중이면 안전한 시점까지 미룹니다.
         * @return 제거를 수락하면 true입니다.
         */
        bool Destroy();

        /**
         * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool InitializeDefaultComponents();

        /**
         * @brief 액터의 게임 실행 시작 시 호출되는 확장 지점입니다.
         * @note 생명주기 상태는 월드의 전달 경로가 관리합니다.
         */
        virtual void BeginPlay();

        /**
         * @brief 기본 액터 Tick 함수가 실행 차례에 호출하는 갱신 확장 지점입니다.
         * @param[in] DeltaTime 호출에 전달된 초 단위 시간 간격입니다.
         */
        virtual void Tick(float DeltaTime);

        /**
         * @brief 액터의 게임 실행 종료 시 호출되는 확장 지점입니다.
         */
        virtual void EndPlay();

        /**
         * @brief 뷰포트 입력을 받아 프레임 시간에 따라 액터의 동작을 갱신합니다.
         * @param[in] Input 현재 프레임의 키보드·마우스 입력 상태입니다.
         * @param[in] DeltaTime 이전 갱신 이후의 경과 시간이며 초 단위입니다.
         */
        virtual void Input(const Core::FInput& Input, float DeltaTime);

        /**
         * @brief 액터가 실행 시작 전달에 진입했는지 검사합니다.
         * @return 실행 시작 전달 이후 종료 전달 전이면 true입니다.
         */
        bool HasBegunPlay() const;

        /**
         * @brief 지연 삭제가 예약되었는지 검사합니다.
         * @return 삭제 요청을 수락한 뒤 실제 해제 전이면 true입니다.
         */
        bool IsPendingDestroy() const;

        /**
         * @brief 정지한 편집 월드에서도 Tick을 허용하는지 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsEditorTickable() const;

        /**
         * @brief 정지한 편집 월드에서의 Tick 허용 여부를 설정합니다.
         * @param[in] EditorTickable 정지한 편집 월드에서 Tick을 허용할지 나타냅니다.
         */
        void SetEditorTickable(bool EditorTickable);

        /** @brief 이 액터의 Tick()을 호출하는 기본 Tick 함수의 참조를 반환합니다. */
        Engine::FActorTickFunction& GetPrimaryActorTick();

        /** @brief 액터의 기본 Tick에 설정된 그룹을 반환합니다. */
        Engine::ETickingGroup GetTickGroup() const;

        /**
         * @brief 액터의 기본 Tick에 설정할 그룹을 지정합니다.
         * @param[in] Group 액터의 기본 Tick이 시작할 수 있는 가장 이른 그룹입니다.
         * @return 지정할 수 있는 그룹이면 true입니다.
         */
        bool SetTickGroup(Engine::ETickingGroup Group);

        /**
         * @brief 이 액터의 기본 Tick을 소속 레벨에 등록하거나 등록을 해제합니다.
         * @param[in] Register 등록할 때 true이고 해제할 때 false입니다.
         * @details 별도의 Tick 함수를 소유하는 파생 클래스는 이 함수를 재정의해 함께 등록하고 해제할 수 있습니다.
         */
        virtual void RegisterActorTickFunctions(bool Register);

        /**
         * @brief 액터의 표시 여부를 반환합니다.
         * @return 액터와 소유 컴포넌트를 표시하도록 설정했으면 true입니다.
         */
        bool IsVisible() const;

        /**
         * @brief 액터의 표시 여부를 변경하고 소유 컴포넌트의 렌더 상태에 반영합니다.
         * @param[in] Visible 액터를 표시하려면 true입니다.
         * @note 렌더링과 에디터 피킹에만 영향을 주며 Tick과 충돌은 그대로 동작합니다.
         */
        void SetVisible(bool Visible);

    private:
        /**
         * @brief 객체가 소속된 월드 연결을 지정합니다.
         * @param[in] World 연결할 월드이며, nullptr이면 연결을 해제합니다.
         * @details 기존 월드에서 액터와 컴포넌트의 Tick 등록을 해제하고 새 월드에 연결된 뒤 다시 등록합니다.
         */
        void SetWorld(Engine::UWorld* World);

        /**
         * @brief 월드가 액터의 실행 전달과 지연 삭제 상태를 관리하도록 허용합니다.
         */
        friend class UWorld;

        /**
         * @brief 레벨이 소유 액터의 월드 연결을 관리하도록 허용합니다.
         */
        friend class ULevel;

        /**
         * @brief 컴포넌트를 먼저 시작한 뒤 살아 있는 액터의 시작 콜백을 전달합니다.
         */
        void DispatchBeginPlay();

        /**
         * @brief 시작된 액터의 종료 콜백과 컴포넌트 역순 종료를 전달합니다.
         */
        void DispatchEndPlay();

        /**
         * @brief 콜백 중 컴포넌트 추가에 영향받지 않는 핸들 스냅샷을 만듭니다.
         * @return 현재 소유 컴포넌트의 전역 핸들 목록입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::UActorComponent>> GetComponentRefs() const;

    private:
        /**
         * @brief 액터가 속한 월드이며, 레벨에 추가되기 전에는 nullptr입니다.
         */
        Engine::UWorld* mWorld{};

        /**
         * @brief 액터가 소유한 컴포넌트 목록입니다.
         */
        Core::TArray<std::unique_ptr<Engine::UActorComponent>> mComponents{};

        FComponentCollisionDelegate mOnComponentCollision{};

        /**
         * @brief 액터 변환을 제공하는 루트 컴포넌트의 비소유 주소입니다.
         */
        Engine::USceneComponent* mRootComponent{};

        /**
         * @brief 액터가 시작 전달에 진입하여 아직 종료되지 않았는지 나타냅니다.
         */
        bool mHasBegunPlay{};

        /**
         * @brief 액터 자체의 BeginPlay 호출 여부이며 부분 시작 시 EndPlay 짝을 구분합니다.
         */
        bool mBeginPlayCalled{};

        /**
         * @brief 안전한 전달 경계에서 액터를 제거하도록 예약한 상태입니다.
         */
        bool mPendingDestroy{};

        /**
         * @brief 월드 연결 변경 중 재진입에 의한 컴포넌트 변경을 제한합니다.
         */
        bool mChangingWorld{};

        /**
         * @brief 정지한 편집 월드에서도 이 객체의 Tick을 허용하는 설정입니다.
         */
        UPROPERTY(Edit, Serialize)
        bool mEditorTickable{};

        /** @brief 액터의 Tick()을 호출하는 기본 함수이며, 설정 그룹을 편집 및 직렬화에 노출합니다. */
        UPROPERTY(Edit, Serialize, Name = "TickGroup", Getter = GetTickGroup, Setter = SetTickGroup)
        Engine::FActorTickFunction mPrimaryActorTick{*this};

        /**
         * @brief 액터와 소유 컴포넌트의 표시 여부이며 아웃라이너에서 변경합니다.
         */
        UPROPERTY(Serialize, Getter = IsVisible, Setter = SetVisible)
        bool mVisible{true};
    };

}
