/**
 * @file FTickFunction.h
 * @brief 레벨에 등록할 Tick 함수와 액터 및 컴포넌트의 기본 Tick 함수를 선언합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Engine/Tick/ETickingGroup.h"
#include "Engine/Tick/FTickTaskLevel.h"
#include "CoreUObject/UObject.h"
#include "Engine/ULevel.h"

#include <memory>

namespace Engine {

    class AActor;
    class UActorComponent;
    class UWorld;
    class FTickTaskManager;

    /**
     * @brief 객체의 갱신 콜백 하나에 등록 상태와 실행 조건을 연결합니다.
     * @details 레벨은 Tick 함수의 포인터를 보관하고, 매니저는 그중 실행 가능한 함수를 프레임 큐에 배치합니다.
     * 파생 클래스는 GetTarget()으로 대상 객체를 제공하고 ExecuteTick()에서 실제 갱신을 수행합니다.
     */
    class FTickFunction {
    private:
        /** @brief 약한 참조가 Tick 함수의 파괴를 감지할 수 있도록 함수가 소유하는 수명 토큰입니다. */
        struct FLifetime {
        };

        /** @brief Tick 함수의 소유권을 얻지 않고 포인터가 가리키는 함수의 생존 여부를 확인합니다. */
        struct FWeakTickFunction {
            /** @brief 수명 토큰이 유효할 때 역참조할 Tick 함수 주소입니다. */
            Engine::FTickFunction* mFunction{};

            /** @brief 주소가 가리키던 Tick 함수의 수명 토큰을 관찰합니다. */
            std::weak_ptr<Engine::FTickFunction::FLifetime> mLifetime{};
        };

    public:
        /** @brief 레벨에 등록되지 않은 기본 설정의 Tick 함수를 초기화합니다. */
        FTickFunction() = default;

        /** @brief 파괴될 함수의 프레임 큐 항목을 무효화하고 레벨 등록을 해제합니다. */
        virtual ~FTickFunction();

        /** @brief 등록 및 실행 큐에서 주소로 식별하는 Tick 함수의 복사 생성을 금지합니다. */
        FTickFunction(const Engine::FTickFunction&) = delete;

        /** @brief 이미 연결한 등록 상태와 실행 큐 위치를 보존하기 위해 복사 대입을 금지합니다. */
        Engine::FTickFunction& operator=(const Engine::FTickFunction&) = delete;

    public:
        /** @brief 선행 조건과 프레임 진행 상황을 적용하기 전의 설정 그룹을 반환합니다. */
        Engine::ETickingGroup GetTickGroup() const;

        /**
         * @brief Tick을 배치할 수 있는 가장 이른 설정 그룹을 지정합니다.
         * @param[in] Group Tick 함수에 지정할 공개 그룹입니다.
         * @return 지정할 수 있는 그룹이면 true입니다.
         * @note 이미 실행 큐에 들어간 Tick의 이번 프레임 그룹은 유지됩니다.
         */
        bool SetTickGroup(Engine::ETickingGroup Group);

        /** @brief 이 함수가 Tick의 실행 후보가 될 수 있는지 반환합니다. */
        bool CanEverTick() const;

        /**
         * @brief 이 함수가 Tick의 실행 후보가 될 수 있는지 설정합니다.
         * @param[in] CanEverTick Tick 후보로 허용하면 true입니다.
         */
        void SetCanEverTick(bool CanEverTick);

        /** @brief 이 함수의 현재 Tick 활성화 설정을 반환합니다. */
        bool IsTickEnabled() const;

        /**
         * @brief Tick의 활성 상태를 설정합니다.
         * @param[in] Enabled 실행을 허용하면 true입니다.
         * @details 일반 프레임은 호출 직전에 이 값을 다시 검사합니다. 프레임 중 새로 활성화된 등록 함수는 남은 그룹에 배치될 수 있습니다.
         */
        void SetTickEnabled(bool Enabled);

        /** @brief 일시정지 실행 목록에 포함될 수 있는지 반환합니다. */
        bool IsTickEvenWhenPaused() const;

        /**
         * @brief 일시정지 상태에서도 이 Tick을 실행할 수 있도록 설정합니다.
         * @param[in] Enabled 일시정지 프레임의 실행 후보로 허용하면 true입니다.
         */
        void SetTickEvenWhenPaused(bool Enabled);

        /**
         * @brief 대상 객체가 속한 레벨의 Tick 목록에 이 함수를 추가합니다.
         * @param[in,out] Level 대상 액터 또는 대상 컴포넌트 소유자의 레벨입니다.
         * @return 등록에 성공했거나 이미 같은 레벨에 등록되어 있으면 true입니다.
         */
        bool RegisterTickFunction(Engine::ULevel& Level);

        /**
         * @brief 레벨의 Tick 목록에서 이 함수를 제거합니다.
         * @details 이미 프레임 실행 큐에 들어간 함수는 대상이 실행 가능한 동안 이번 프레임에 호출될 수 있습니다.
         * 아직 큐에 들어가지 않은 신규 등록은 그룹 배치 시점의 등록 여부 검사에서 제외됩니다.
         */
        void UnregisterTickFunction();

        /** @brief 이 함수가 현재 레벨의 Tick 목록에 등록되어 있는지 반환합니다. */
        bool IsTickFunctionRegistered() const;

        /**
         * @brief 이 함수보다 먼저 실행되어야 하는 Tick을 추가합니다.
         * @param[in] Prerequisite 이 함수가 기다리는 Tick입니다.
         * @return 추가했거나 이미 같은 선행 조건이 있으면 true이며, 순환이 생기면 false입니다.
         * @details 두 함수가 배치 대상이면 이 함수의 실행 그룹이 선행 Tick의 실행 그룹까지 늦춰질 수 있습니다.
         */
        bool AddPrerequisite(Engine::FTickFunction& Prerequisite);

        /**
         * @brief 지정한 선행 Tick과의 의존 관계를 제거합니다.
         * @param[in] Prerequisite 제거할 선행 Tick입니다.
         * @return 선행 조건을 제거했으면 true입니다.
         */
        bool RemovePrerequisite(const Engine::FTickFunction& Prerequisite);

    private:
        /** @brief 레벨 목록을 정리할 때 등록 상태를 해제할 수 있습니다. */
        friend class FTickTaskLevel;

        /** @brief 실행 큐를 구성할 때 이 함수의 선행 조건과 프레임 배치 상태에 접근합니다. */
        friend class FTickTaskManager;

        /** @brief 이 Tick의 등록과 실행 조건을 확인할 대상 객체를 반환합니다. */
        virtual const CoreUObject::UObject* GetTarget() const = 0;

        /**
         * @brief 대상이 주어진 월드에서 실행할 수 있는 상태인지 확인합니다.
         * @param[in] World 현재 갱신할 월드입니다.
         * @param[in] EditorTick 정지한 편집 월드의 갱신이면 true입니다.
         * @return 월드 소속과 대상의 수명 및 실행 상태가 맞으면 true입니다.
         */
        virtual bool CanExecuteTick(const Engine::UWorld& World, bool EditorTick) const;

        /**
         * @brief 프레임 큐에서 차례가 되었을 때 실행할 갱신 콜백입니다.
         * @param[in] DeltaTime 현재 갱신에 전달된 초 단위 시간 간격입니다.
         */
        virtual void ExecuteTick(float DeltaTime) = 0;

        /** @brief 프레임 큐에 저장된 함수 포인터를 비우고 이 함수의 배치 상태를 초기화합니다. */
        void ClearQueuedTick();

        /** @brief 등록된 함수가 실행 후보가 되면 레벨의 활성 매니저에 알립니다. */
        void NotifyTickEnabled();

        /** @brief 수명 토큰을 필요할 때 만들고 함수 주소와 토큰을 담은 약한 참조를 반환합니다. */
        Engine::FTickFunction::FWeakTickFunction GetWeakReference();

        /**
         * @brief 유효한 선행 조건을 따라 지정한 함수에 도달할 수 있는지 검사합니다.
         * @param[in] Function 순환 검사에서 찾으려는 Tick 함수입니다.
         * @return 자신이 지정한 함수이거나 선행 조건을 거쳐 도달할 수 있으면 true입니다.
         */
        bool DependsOn(const Engine::FTickFunction& Function) const;

    private:
        /** @brief 의존성이나 프레임 진행 상황을 적용하기 전의 설정 그룹입니다. */
        Engine::ETickingGroup mTickGroup{Engine::ETickingGroup::PrePhysics};

        /** @brief 이 함수가 Tick 실행 후보가 될 수 있는지 나타냅니다. */
        bool mCanEverTick{true};

        /** @brief 현재 Tick 실행이 활성화되어 있는지 나타냅니다. */
        bool mTickEnabled{true};

        /** @brief 일시정지 후보 선정에 이 함수를 포함할 수 있는지 나타냅니다. */
        bool mTickEvenWhenPaused{};

        /** @brief 이 함수가 등록된 레벨의 목록입니다. 등록 해제 시 nullptr입니다. */
        Engine::FTickTaskLevel* mTaskLevel{};

        /** @brief 등록 해제 시 함수 포인터를 제거할 목록 위치입니다. */
        Engine::FTickTaskLevel::FTickList::iterator mRegistrationPosition{};

        /** @brief 이번 프레임에 배치한 큐 칸의 주소이며, 중복 배치를 막기 위해 호출 후에도 유지합니다. */
        Engine::FTickFunction** mQueuedSlot{};

        /** @brief 현재 큐를 구성한 월드이며, 다른 월드에 재등록할 때 이전 큐 위치를 지우는 데 사용합니다. */
        const Engine::UWorld* mQueuedWorld{};

        /** @brief 이번 프레임에 배치된 실제 그룹입니다. 그룹이 없는 일시정지 큐에도 Count를 사용합니다. */
        Engine::ETickingGroup mQueuedGroup{Engine::ETickingGroup::Count};

        /** @brief 함수가 파괴되면 만료되는 약한 참조의 수명 토큰입니다. */
        std::shared_ptr<Engine::FTickFunction::FLifetime> mLifetime{};

        /** @brief 이 함수보다 먼저 배치할 Tick의 약한 참조 목록입니다. */
        Core::TArray<Engine::FTickFunction::FWeakTickFunction> mPrerequisites{};
    };

    /** @brief 실행 차례를 대상 액터의 AActor::Tick() 호출로 전달하는 기본 Tick 함수입니다. */
    class FActorTickFunction final : public Engine::FTickFunction {
    public:
        /**
         * @brief 기본 Tick 함수를 액터에 연결합니다.
         * @param[in,out] Target Tick()을 전달받을 액터입니다.
         */
        explicit FActorTickFunction(Engine::AActor& Target);

    private:
        /** @brief 등록과 실행 조건 확인에 사용할 액터를 반환합니다. */
        const CoreUObject::UObject* GetTarget() const override;

        /**
         * @brief 대상 액터의 월드 소속, 파괴 대기 상태 및 해당 모드의 실행 조건을 검사합니다.
         * @param[in] World Tick을 전달하는 월드입니다.
         * @param[in] EditorTick 정지한 편집 월드의 갱신이면 true입니다.
         * @return 대상 액터가 실행 가능하면 true입니다.
         */
        bool CanExecuteTick(const Engine::UWorld& World, bool EditorTick) const override;

        /**
         * @brief 액터 Tick 통계를 집계하고 mTarget의 AActor::Tick()을 호출합니다.
         * @param[in] DeltaTime 대상 액터에 전달할 초 단위 시간 간격입니다.
         */
        void ExecuteTick(float DeltaTime) override;

    private:
        /** @brief 이 기본 Tick이 갱신할 액터의 비소유 참조입니다. */
        Engine::AActor& mTarget;
    };

    /** @brief 실행 차례를 대상 컴포넌트의 UActorComponent::Tick() 호출로 전달하는 기본 Tick 함수입니다. */
    class FActorComponentTickFunction final : public Engine::FTickFunction {
    public:
        /**
         * @brief 기본 Tick 함수를 컴포넌트에 연결합니다.
         * @param[in,out] Target Tick()을 전달받을 컴포넌트입니다.
         */
        explicit FActorComponentTickFunction(Engine::UActorComponent& Target);

    private:
        /** @brief 등록과 실행 조건 확인에 사용할 컴포넌트를 반환합니다. */
        const CoreUObject::UObject* GetTarget() const override;

        /**
         * @brief 소유 액터의 기본 실행 조건과 컴포넌트 자체의 실행 조건을 검사합니다.
         * @param[in] World Tick을 전달하는 월드입니다.
         * @param[in] EditorTick 정지한 편집 월드의 갱신이면 true입니다.
         * @return 대상 컴포넌트가 실행 가능하면 true입니다.
         */
        bool CanExecuteTick(const Engine::UWorld& World, bool EditorTick) const override;

        /**
         * @brief 컴포넌트 Tick 통계를 집계하고 mTarget의 UActorComponent::Tick()을 호출합니다.
         * @param[in] DeltaTime 대상 컴포넌트에 전달할 초 단위 시간 간격입니다.
         */
        void ExecuteTick(float DeltaTime) override;

    private:
        /** @brief 이 기본 Tick이 갱신할 컴포넌트의 비소유 참조입니다. */
        Engine::UActorComponent& mTarget;
    };

}
