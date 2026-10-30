/**
 * @file UWorld.h
 * @brief 영속 레벨, 렌더 장면과 프레임 말 컴포넌트 갱신을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"
#include "Engine/EWorldType.h"
#include "Engine/EWorldPlayState.h"
#include "Engine/FWorldHandle.h"

namespace Core {
    struct FRay;
}

namespace Engine {

    class ULevel;
    class FAssetManager;
    class AActor;
    class FWorldContext;
    class FSceneInterface;
    class AStaticMeshActor;
    class UCameraComponent;
    struct FViewportInput;
    struct FWorldHit;
    enum class EPrimitiveType : Core::Uint8;

    /**
     * @brief 영속 레벨, 렌더 장면과 프레임 말 컴포넌트 갱신을 관리합니다.
     * @ingroup Engine
     */
    class UWorld final : public CoreUObject::UObject {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(UWorld, CoreUObject::UObject);

    public:
        /**
         * @brief 중첩 콜백 동안 객체 해제를 지연하는 월드 전달 범위입니다.
         * @note 가장 바깥 범위가 끝나면 지연된 종료와 액터 삭제를 처리합니다.
         */
        class FDispatchScope {
        public:
            /**
             * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
             */
            ReflectTypes(FDispatchScope);

        public:
            /**
             * @brief 월드의 콜백 전달 깊이를 증가시킵니다.
             * @param[in] World 보호할 월드의 비소유 주소이며 nullptr이면 아무 작업도 하지 않습니다.
             */
            explicit FDispatchScope(Engine::UWorld* World);

            /**
             * @brief 전달 깊이를 감소시키고 안전한 경계에서 지연 작업을 처리합니다.
             */
            ~FDispatchScope();

            /**
             * @brief 전달 깊이가 중복 관리되지 않도록 범위 복사를 금지합니다.
             */
            FDispatchScope(const FDispatchScope&) = delete;

            /**
             * @brief 전달 깊이가 중복 관리되지 않도록 범위 복사 대입을 금지합니다.
             */
            FDispatchScope& operator=(const FDispatchScope&) = delete;

        public:
            /**
             * @brief 전달 범위의 정적 구조체 메타데이터를 반환합니다.
             * @return 등록된 메타데이터의 비소유 주소입니다.
             */
            ReflectStruct(FDispatchScope);

        private:
            /**
             * @brief 전달 범위의 리플렉션 등록 함수와 빌더 접근 권한입니다.
             */
            ReflectMembers();

        private:
            /**
             * @brief 전달 깊이를 관리하는 월드의 비소유 주소입니다.
             */
            Engine::UWorld* mWorld{};
            ReflectProperty(mWorld, "World", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        };

    public:
        /**
         * @brief UWorld의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        UWorld();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UWorld() override;

    public:
        /**
         * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UWorld, CoreUObject::EClassFlags::NoFactory);

        /**
         * @brief 이 월드를 엔진에 등록할 때 발급한 핸들을 반환합니다.
         * @return 엔진 등록 전에는 무효인 월드 핸들입니다.
         */
        Engine::FWorldHandle GetWorldHandle() const;

        /**
         * @brief 월드의 편집 또는 실행 용도를 반환합니다.
         * @return 컨텍스트 생성 시 지정한 월드 종류입니다.
         */
        Engine::EWorldType GetWorldType() const;

        /**
         * @brief 현재 게임 실행 생명주기 상태를 반환합니다.
         * @return 시작, 실행, 일시정지 또는 종료 상태입니다.
         */
        Engine::EWorldPlayState GetPlayState() const;

        /**
         * @brief 정지 상태의 Game 또는 PIE 월드에서 게임 생명주기를 시작합니다.
         * @return 시작 전달을 수행하면 true이며 잘못된 상태나 재진입은 false입니다.
         * @note 누적 시간과 Tick 횟수를 초기화합니다. 콜백에서 종료를 요청하면 전달 범위를 마칠 때 종료될 수 있습니다.
         */
        bool BeginPlay();

        /**
         * @brief 시작된 액터와 컴포넌트의 실행을 종료합니다.
         * @note 콜백 전달 중에는 종료를 예약하며 중복 호출해도 종료 콜백을 반복하지 않습니다.
         */
        void EndPlay();

        /**
         * @brief Playing 상태의 게임 갱신 또는 정지한 편집 월드의 선택적 갱신을 수행합니다.
         * @param[in] DeltaTime 진행할 초 단위 시간이며 유한한 0 이상의 값이어야 합니다.
         * @return 갱신 전달을 수행하면 true이며 편집 Tick은 게임 시간과 횟수에 합산하지 않습니다.
         */
        bool Tick(float DeltaTime);

        /**
         * @brief Playing과 Paused 사이의 상태를 전환합니다.
         * @param[in] Paused true이면 일시정지하고 false이면 재개합니다.
         * @return 실행 또는 일시정지 상태에서 요청을 수락하면 true입니다.
         */
        bool SetPaused(bool Paused);

        /**
         * @brief Paused 상태를 유지하면서 게임 갱신을 한 번 수행합니다.
         * @param[in] DeltaTime 진행할 초 단위 시간이며 유한한 양수여야 합니다.
         * @return 일시정지 상태에서 한 번 갱신하면 true입니다.
         */
        bool Step(float DeltaTime);

        /**
         * @brief 마지막 실행 시작 이후 누적 게임 시간을 반환합니다.
         * @return Tick과 Step이 실제 진행한 초 단위 시간입니다.
         */
        float GetTimeSeconds() const;

        /**
         * @brief 마지막 실행 시작 이후 수행한 게임 갱신 횟수를 반환합니다.
         * @return 실제 처리한 Tick과 Step의 총횟수입니다.
         */
        Core::Uint64 GetTickCount() const;

        /**
         * @brief 콜백 전달 또는 지연된 액터 정리를 처리 중인지 검사합니다.
         * @return 월드의 즉시 구조 변경을 제한해야 하는 동안 true입니다.
         */
        bool IsDispatching() const;

        /**
         * @brief 월드 소속을 검증하여 활성 카메라를 선택하거나 해제합니다.
         * @param[in] Camera 활성화할 이 월드 소속 카메라이며 nullptr이면 선택을 해제합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetActiveCamera(Engine::UCameraComponent* Camera);

        /**
         * @brief 월드에 등록된 활성 카메라를 핸들로 검증하여 조회합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
         */
        Engine::UCameraComponent* GetActiveCamera() const;

        /**
         * @brief 월드 소속을 검증하여 입력 대상 액터를 선택하거나 해제합니다.
         * @param[in,out] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetInputTarget(Engine::AActor* Actor);

        /**
         * @brief 월드의 뷰포트 입력을 받을 액터를 핸들로 검증하여 조회합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
         */
        Engine::AActor* GetInputTarget() const;

        /**
         * @brief 뷰포트 입력을 받아 프레임 시간에 따라 액터의 동작을 갱신합니다.
         * @param[in] Input 현재 프레임의 키보드·마우스 입력 상태입니다.
         * @param[in] DeltaTime 이전 갱신 이후의 경과 시간이며 초 단위입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Input(const Engine::FViewportInput& Input, float DeltaTime);

        /**
         * @fn UClass* UWorld::GetClass() const
         * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
         * @return 실제 클래스 메타데이터의 비소유 주소입니다.
         */
        /**
         * @brief 아직 레벨이 없는 월드에 영속 레벨의 소유권을 연결합니다.
         * @param[in] Level 소유권을 넘겨받을 영속 레벨입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool SetPersistentLevel(std::unique_ptr<Engine::ULevel> Level);

        /**
         * @brief 월드가 소유한 영속 레벨을 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::ULevel* GetPersistentLevel() const;

        /**
         * @brief 객체의 실제 소유 관계가 이 월드에 속하는지 검사합니다.
         * @param[in] Object 소속을 확인할 객체의 비소유 주소입니다.
         * @return 이 월드, 영속 레벨 또는 살아 있는 소유 액터와 컴포넌트이면 true입니다.
         */
        bool ContainsObject(const CoreUObject::UObject* Object) const;

        /**
         * @brief 월드에 렌더 장면을 연결하고 기존 프리미티브를 동기화합니다.
         * @param[in] Scene 연결하거나 렌더링할 장면입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool AttachScene(std::unique_ptr<Engine::FSceneInterface> Scene);

        /**
         * @brief 연결된 렌더 장면을 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::FSceneInterface* GetScene() const;

        /**
         * @brief 월드의 에셋 로드에 사용할 관리자의 비소유 주소를 설정합니다.
         * @param[in] Assets 월드가 사용할 에셋 관리자의 비소유 주소입니다.
         */
        void SetAssets(Engine::FAssetManager* Assets);

        /**
         * @brief 프로젝트의 에셋 관리자를 비소유 주소로 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
         */
        Engine::FAssetManager* GetAssets() const;

        /**
         * @brief 지정한 액터 클래스의 기본 컴포넌트를 생성하고 영속 레벨에 추가합니다.
         * @param[in] Class 팩터리를 제공하는 AActor 파생 클래스 메타데이터입니다.
         * @return 생성 후 살아 있는 액터의 비소유 주소이며 실패하거나 시작 콜백에서 삭제되면 nullptr입니다.
         * @note 실행 중 추가된 액터는 즉시 시작되며 이미 진행 중인 Tick 스냅샷에는 포함되지 않습니다.
         */
        Engine::AActor* SpawnActor(const CoreUObject::UClass* Class);

        /**
         * @brief 지정한 기본 메시 종류의 액터를 생성하여 영속 레벨에 추가합니다.
         * @param[in] Type 생성, 조회 또는 표시할 타입입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::AStaticMeshActor* SpawnPrimitive(Engine::EPrimitiveType Type);

        /**
         * @brief 핸들에 해당하는 액터의 컴포넌트를 등록 해제하고 액터를 제거합니다.
         * @param[in] Handle 조회 또는 제거할 객체의 슬롯 세대 핸들입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool DestroyActor(CoreUObject::FObjectHandle Handle);

        /**
         * @brief 컴포넌트 핸들을 프레임 말 렌더 갱신 큐에 추가합니다.
         * @param[in] Handle 조회 또는 제거할 객체의 슬롯 세대 핸들입니다.
         */
        void EnqueueRenderUpdate(CoreUObject::FObjectHandle Handle);

        /**
         * @brief 큐에 쌓인 컴포넌트 핸들을 해석하여 지연된 렌더 갱신을 반영합니다.
         */
        void SendAllEndOfFrameUpdates();

        /**
         * @brief 월드 프리미티브의 메시를 검사하여 광선에 가장 가까운 충돌을 찾습니다.
         * @param[in] Ray 교차를 검사하거나 역투영 결과를 기록할 광선입니다.
         * @param[in,out] Hit 교차 성공 시 충돌 위치와 거리를 기록합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         * @note 입력 방향을 내부에서 정규화합니다. 원점이 유한하지 않거나 방향을 정규화할 수 없으면 실패합니다.
         */
        bool Raycast(const Core::FRay& Ray, Engine::FWorldHit& Hit) const;

        /**
         * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
         */
        void Shutdown();

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 컨텍스트가 소유 월드의 종류와 엔진 식별자를 설정하도록 허용합니다.
         */
        friend class FWorldContext;

        /**
         * @brief 복제기가 원본과 복제본의 전달 범위를 보호하도록 허용합니다.
         */
        friend class FWorldCloner;

        /**
         * @brief 액터가 월드의 전달 범위와 실행 가능 상태를 검사하도록 허용합니다.
         */
        friend class AActor;

        /**
         * @brief 컴포넌트가 등록과 실행 콜백의 안전한 전달 범위를 사용하도록 허용합니다.
         */
        friend class UActorComponent;

        /**
         * @brief 프리미티브가 렌더 상태 등록과 해제를 전달 범위로 보호하도록 허용합니다.
         */
        friend class UPrimitiveComponent;

        /**
         * @brief 레벨이 액터 추가와 제거를 월드 생명주기에 연결하도록 허용합니다.
         */
        friend class ULevel;

        /**
         * @brief 전달 깊이를 줄이고 안전한 경계에서 지연 작업을 처리합니다.
         */
        void FinishDispatch();

        /**
         * @brief 가장 바깥 전달 범위 이후 종료 요청과 액터 삭제 및 월드 종료를 처리합니다.
         */
        void FlushPendingActors();

        /**
         * @brief 월드 상태에 맞추어 새 액터의 시작 콜백을 전달합니다.
         * @param[in,out] Actor 영속 레벨에 추가된 액터입니다.
         */
        void NotifyActorAdded(Engine::AActor* Actor);

        /**
         * @brief 월드가 새 액터나 컴포넌트 등록을 수락할 수 있는지 검사합니다.
         * @return 종료 중이 아니고 종료 요청도 없다면 true입니다.
         */
        bool CanAddActor() const;

        /**
         * @brief 월드가 액터 시작 콜백을 전달할 수 있는지 검사합니다.
         * @return 시작, 실행 또는 일시정지 중이며 종료 요청이 없다면 true입니다.
         */
        bool CanBeginActor() const;

        /**
         * @brief 시간을 검증하고 액터 핸들 스냅샷으로 갱신을 전달합니다.
         * @param[in] DeltaTime 진행할 유한한 0 이상의 초 단위 시간입니다.
         * @param[in] EditorTick 객체별 편집 Tick 설정을 검사하고 게임 시간은 유지할지 지정합니다.
         * @return 재진입 없이 갱신 전달을 수행하면 true입니다.
         */
        bool TickActors(float DeltaTime, bool EditorTick);

        /**
         * @brief 컨테이너 변경에 영향받지 않는 액터 핸들 스냅샷을 만듭니다.
         * @return 영속 레벨에 속한 액터의 전역 핸들 목록입니다.
         */
        Core::TArray<CoreUObject::FObjectHandle> GetActorHandles() const;

        /**
         * @brief 전역 핸들을 해석하고 이 월드의 액터인지 확인합니다.
         * @param[in] Handle 전역 레지스트리에서 조회할 객체 핸들입니다.
         * @return 소속이 일치하는 액터의 비소유 주소이며 없으면 nullptr입니다.
         */
        Engine::AActor* ResolveActor(CoreUObject::FObjectHandle Handle) const;

    private:
        /**
         * @brief 활성 카메라 컴포넌트의 비소유 객체 핸들입니다.
         */
        CoreUObject::FObjectHandle mActiveCamera{};
        ReflectProperty(mActiveCamera, "ActiveCamera", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프로젝트 에셋을 조회할 관리자의 비소유 주소입니다.
         */
        Engine::FAssetManager* mAssets{};
        ReflectProperty(mAssets, "Assets", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 뷰포트 입력을 전달할 액터의 비소유 객체 핸들입니다.
         */
        CoreUObject::FObjectHandle mInputTarget{};
        ReflectProperty(mInputTarget, "InputTarget", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 엔진이 발급한 이 월드의 비소유 식별자입니다.
         */
        Engine::FWorldHandle mWorldHandle{};
        ReflectProperty(mWorldHandle, "WorldHandle", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 컨텍스트 생성 시 지정하는 월드의 용도입니다.
         */
        Engine::EWorldType mWorldType{Engine::EWorldType::Editor};
        ReflectProperty(mWorldType, "WorldType", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드가 관리하는 현재 게임 실행 상태입니다.
         */
        Engine::EWorldPlayState mPlayState{Engine::EWorldPlayState::Stopped};
        ReflectProperty(mPlayState, "PlayState", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드가 소유한 영속 레벨입니다.
         */
        std::unique_ptr<Engine::ULevel> mPersistentLevel{};
        ReflectProperty(mPersistentLevel, "PersistentLevel", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드가 소유한 렌더 장면입니다.
         */
        std::unique_ptr<Engine::FSceneInterface> mScene{};
        ReflectProperty(mScene, "Scene", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프레임 말 갱신을 기다리는 컴포넌트 핸들입니다.
         */
        Core::TArray<CoreUObject::FObjectHandle> mDirtyComponents{};
        ReflectProperty(mDirtyComponents, "DirtyComponents", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 가장 바깥 콜백 전달 종료 후 제거할 액터 핸들입니다.
         */
        Core::TArray<CoreUObject::FObjectHandle> mPendingActors{};
        ReflectProperty(mPendingActors, "PendingActors", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 기본 액터 이름 생성용 일련번호입니다.
         */
        Core::Uint32 mSpawnSerial{};
        ReflectProperty(mSpawnSerial, "SpawnSerial", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 중첩된 콜백 전달 범위의 깊이입니다.
         */
        Core::Uint32 mDispatchDepth{};
        ReflectProperty(mDispatchDepth, "DispatchDepth", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 마지막 BeginPlay 이후 실제 처리한 게임 갱신 횟수입니다.
         */
        Core::Uint64 mTickCount{};
        ReflectProperty(mTickCount, "TickCount", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 마지막 BeginPlay 이후 실제 진행한 게임 시간입니다.
         */
        float mTimeSeconds{};
        ReflectProperty(mTimeSeconds, "TimeSeconds", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 지연된 정리 작업의 재진입을 막는 처리 상태입니다.
         */
        bool mFlushing{};
        ReflectProperty(mFlushing, "Flushing", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 콜백 범위 종료 후 처리할 실행 종료 요청입니다.
         */
        bool mEndPlayRequested{};
        ReflectProperty(mEndPlayRequested, "EndPlayRequested", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 콜백과 지연 삭제를 마친 뒤 처리할 월드 종료 요청입니다.
         */
        bool mShutdownRequested{};
        ReflectProperty(mShutdownRequested, "ShutdownRequested", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드 리소스를 정리하며 새 등록을 거부하는 상태입니다.
         */
        bool mShuttingDown{};
        ReflectProperty(mShuttingDown, "ShuttingDown", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
