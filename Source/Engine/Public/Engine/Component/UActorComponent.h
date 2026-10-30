/**
 * @file UActorComponent.h
 * @brief 소유 액터와 월드 등록 수명을 갖는 컴포넌트의 기반 클래스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"

namespace Engine {

    class AActor;
    class UWorld;

    /**
     * @brief 소유 액터와 월드 등록 수명을 갖는 컴포넌트의 기반 클래스입니다.
     * @ingroup Engine
     */
    class UActorComponent : public CoreUObject::UObject {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(UActorComponent, CoreUObject::UObject);

    public:
        /**
         * @brief UActorComponent의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        UActorComponent();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UActorComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UActorComponent);

        /**
         * @fn UClass* UActorComponent::GetClass() const
         * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
         * @return 실제 클래스 메타데이터의 비소유 주소입니다.
         */
        /**
         * @brief 소유 객체 또는 메타데이터를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::AActor* GetOwner() const;

        /**
         * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::UWorld* GetWorld() const;

        /**
         * @brief 컴포넌트를 월드에 연결하고 필요한 렌더 상태를 등록합니다.
         * @param[in,out] World 연결할 월드 또는 적용할 월드 행렬입니다.
         */
        virtual void RegisterComponent(Engine::UWorld* World);

        /**
         * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
         */
        virtual void UnregisterComponent();

        /**
         * @brief 컴포넌트가 월드에 등록되어 있는지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsRegistered() const;

        /**
         * @brief 컴포넌트의 게임 실행 시작 시 호출되는 확장 지점입니다.
         * @note 생명주기 상태는 월드의 전달 경로가 관리합니다.
         */
        virtual void BeginPlay();

        /**
         * @brief 컴포넌트의 실행 또는 허용된 편집 갱신 시 호출되는 확장 지점입니다.
         * @param[in] DeltaTime 이번 갱신이 진행하는 초 단위 시간입니다.
         */
        virtual void Tick(float DeltaTime);

        /**
         * @brief 컴포넌트의 게임 실행 종료 시 호출되는 확장 지점입니다.
         */
        virtual void EndPlay();

        /**
         * @brief 컴포넌트가 실행 시작 전달에 진입했는지 검사합니다.
         * @return 실행 시작 전달 이후 종료 전달 전이면 true입니다.
         */
        bool HasBegunPlay() const;

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

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();
        virtual void OnRegister();

        /**
         * @brief 컴포넌트의 소유 액터를 지정합니다.
         * @param[in] Owner 컴포넌트 또는 메타데이터의 소유자입니다.
         */
        void SetOwner(Engine::AActor* Owner);

        /**
         * @brief 소유 액터가 컴포넌트의 소유 관계와 실행 전달을 관리하도록 허용합니다.
         */
        friend class AActor;

        /**
         * @brief 시작 상태를 설정하고 중복되지 않도록 시작 콜백을 전달합니다.
         */
        void DispatchBeginPlay();

        /**
         * @brief 등록된 컴포넌트의 실행 상태 또는 편집 Tick 설정에 따라 갱신을 전달합니다.
         * @param[in] DeltaTime 이번 갱신의 초 단위 시간입니다.
         * @param[in] EditorTick BeginPlay 없이 편집 Tick 설정을 검사할지 지정합니다.
         */
        void DispatchTick(float DeltaTime, bool EditorTick);

        /**
         * @brief 시작 상태를 먼저 해제하여 종료 콜백의 중복 전달을 막습니다.
         */
        void DispatchEndPlay();

    private:
        /**
         * @brief 소유 객체 또는 메타데이터입니다.
         */
        Engine::AActor* mOwner{};
        ReflectProperty(mOwner, "Owner", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드 연결 또는 월드 변환 행렬입니다.
         */
        Engine::UWorld* mWorld{};
        ReflectProperty(mWorld, "World", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 컴포넌트가 시작 전달에 진입하여 아직 종료되지 않았는지 나타냅니다.
         */
        bool mHasBegunPlay{};
        ReflectProperty(mHasBegunPlay, "HasBegunPlay", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정지한 편집 월드에서도 이 객체의 Tick을 허용하는 설정입니다.
         */
        bool mEditorTickable{};
        ReflectProperty(mEditorTickable, "EditorTickable", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
    };

}
