/**
 * @file FLuaRuntime.h
 * @brief 월드의 Lua 스크립트 컴포넌트를 실행하는 런타임과 컴포넌트별 실행 인스턴스를 선언합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/FPathTracker.h"
#include "Core/Input.h"
#include "CoreUObject/TObjectRef.h"
#include "Engine/AActor.h"
#include "Engine/Collision/FHitResult.h"
#include "Script/FLuaVM.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/ULuaScriptComponent.h"
#include "Engine/UWorld.h"

#include <map>
#include <vector>

namespace Engine {

    class UProjectileMovementComponent;

    /**
     * @brief Lua 스크립트 컴포넌트 하나의 실행 환경, 수명 주기 콜백과 대기 중인 충돌 이벤트를 보관합니다.
     * @ingroup Engine
     * @note FLuaRuntime만 생성하고 조작합니다.
     */
    class FLuaScriptInstance {
    private:
        /**
         * @brief 다음 Tick에서 스크립트에 전달할 충돌 이벤트입니다.
         */
        struct FEvent {
            /**
             * @brief 겹침 시작, 겹침 종료 또는 충돌 중 이벤트의 종류입니다.
             */
            Engine::AActor::EComponentCollision mKind{};

            /**
             * @brief 충돌한 상대 액터입니다.
             */
            CoreUObject::TObjectRef<Engine::AActor> mOther{};

            /**
             * @brief 충돌 정보이며 충돌 이벤트가 아니면 기본값입니다.
             */
            Engine::FHitResult mHit{};
        };

    private:
        friend class FLuaRuntime;

        /**
         * @brief 소유 액터의 충돌 델리게이트에서 이벤트 수집 처리기를 제거합니다.
         */
        void DisconnectCollision();

    private:
        /**
         * @brief 실행 중인 스크립트 파일 경로입니다.
         */
        std::filesystem::path mPath{};

        /**
         * @brief 이 인스턴스를 소유하는 스크립트 컴포넌트입니다.
         */
        CoreUObject::TObjectRef<Engine::ULuaScriptComponent> mComponent{};

        /**
         * @brief 스크립트의 Actor로 노출되고 충돌 이벤트를 받는 소유 액터입니다.
         */
        CoreUObject::TObjectRef<Engine::AActor> mOwner{};

        /**
         * @brief 소유 액터의 충돌 델리게이트에 등록한 처리기 핸들입니다.
         */
        CoreUObject::FDelegateHandle mCollisionHandle{};

        /**
         * @brief 현재 실행 환경과 콜백입니다.
         */
        Script::FLuaExecution mExecution{};

        /**
         * @brief 마지막 Tick 이후 수집한 충돌 이벤트입니다.
         */
        std::vector<FEvent> mEvents{};

        /**
         * @brief 스크립트가 실행 중인지 나타내며 콜백 오류가 나면 false가 되어 이후 호출을 멈춥니다.
         */
        bool mRunning{};
    };

    /**
     * @brief 월드 하나의 Lua 상태를 소유하고 스크립트 컴포넌트의 시작, 갱신, 종료와 핫 리로드를 처리합니다.
     * @ingroup Engine
     * @note 같은 파일을 쓰는 컴포넌트는 컴파일된 청크를 공유하지만 각자 별도의 전역 환경에서 실행합니다. 스크립트 오류는 경고로 보고하고 해당 인스턴스만 멈춥니다.
     */
    class FLuaRuntime {
    private:
        /**
         * @brief Lua에 Actor 타입으로 노출하는 액터 약한 참조입니다.
         */
        struct FActorRef {
            /**
             * @brief 참조하는 액터입니다.
             */
            CoreUObject::TObjectRef<Engine::AActor> mActor{};

            /**
             * @brief 참조하는 액터를 반환합니다.
             * @return 유효한 액터입니다.
             * @exception sol::error 액터가 사라졌거나 파괴 대기 중이면 발생하며 Lua 오류로 전달됩니다.
             */
            Engine::AActor& Resolve() const;
        };

        /**
         * @brief Lua에 공통 Component 타입으로 노출하는 액터 컴포넌트 약한 참조입니다.
         */
        class FLuaComponentRef {
        public:
            /**
             * @brief 엔진이 소유한 컴포넌트를 Lua에서 참조할 수 있게 감쌉니다.
             * @param[in] Component 참조할 컴포넌트입니다.
             */
            explicit FLuaComponentRef(Engine::UActorComponent* Component);

        public:
            /**
             * @brief 컴포넌트와 소유 액터가 유효할 때 컴포넌트 주소를 반환합니다.
             * @return 컴포넌트가 사라졌거나 소유 액터가 유효하지 않으면 nullptr입니다.
             */
            Engine::UActorComponent* Get() const;

            /**
             * @brief Lua 바인딩에서 사용할 유효한 컴포넌트를 반환합니다.
             * @exception sol::error Get()이 nullptr을 반환하면 발생하며 Lua 오류로 전달됩니다.
             */
            Engine::UActorComponent& Resolve() const;

        private:
            CoreUObject::TObjectRef<Engine::UActorComponent> mComponent{};
        };

        /**
         * @brief 스크립트 파일 하나의 컴파일 결과와 사용 수입니다.
         */
        struct FSource {
            /**
             * @brief 컴파일한 청크이며 읽기나 컴파일에 실패하면 유효하지 않습니다.
             */
            sol::protected_function mChunk{};

            /**
             * @brief 이 파일을 실행 중인 인스턴스 수이며 0이 되면 항목과 파일 추적을 제거합니다.
             */
            std::size_t mUsers{};
        };

    public:
        /**
         * @brief Lua 상태를 열고 엔진 바인딩과 부트스트랩 스크립트를 등록합니다.
         * @param[in] World 런타임을 소유하는 월드이며 런타임보다 오래 유지되어야 합니다.
         */
        explicit FLuaRuntime(Engine::UWorld& World);

        /**
         * @brief 남은 인스턴스의 충돌 연결과 컴포넌트의 인스턴스 포인터를 정리합니다.
         * @note EndPlay 콜백은 호출하지 않습니다.
         */
        ~FLuaRuntime();

    public:
        /**
         * @brief 컴포넌트의 스크립트를 불러와 실행 환경을 만들고 BeginPlay를 호출합니다.
         * @param[in,out] Component 스크립트를 실행할 컴포넌트입니다.
         * @param[in] Path 실행할 스크립트 파일 경로이며 변경 감시 대상에 추가됩니다.
         * @return 컴포넌트의 실행 인스턴스이며 이미 시작했으면 기존 인스턴스, 소유 액터가 없으면 nullptr입니다.
         * @note 스크립트 오류가 있어도 인스턴스를 만들며 이때는 실행하지 않는 상태로 남아 다음 핫 리로드를 기다립니다.
         */
        Engine::FLuaScriptInstance* Start(Engine::ULuaScriptComponent& Component, const std::filesystem::path& Path);

        /**
         * @brief 수집한 충돌 이벤트 콜백, Tick 콜백을 호출하고 코루틴 작업을 진행합니다.
         * @param[in,out] Instance 갱신할 실행 인스턴스입니다.
         * @param[in] DeltaTime 이전 프레임 이후의 초 단위 경과 시간입니다.
         * @note 실행 중이 아니면 아무것도 하지 않으며 콜백 오류가 나면 남은 호출을 건너뛰고 실행을 멈춥니다.
         */
        static void Tick(Engine::FLuaScriptInstance& Instance, float DeltaTime);

        /**
         * @brief 충돌 연결을 끊고 EndPlay를 호출한 뒤 인스턴스를 제거합니다.
         * @param[in] Component 인스턴스를 소유하는 컴포넌트입니다.
         * @param[in,out] Instance Start()가 반환한 실행 인스턴스이며 호출 후 무효가 됩니다.
         */
        void Stop(Engine::ULuaScriptComponent& Component, Engine::FLuaScriptInstance& Instance);

        /**
         * @brief 디스크에서 바뀐 스크립트 파일을 다시 컴파일해 그 파일을 쓰는 인스턴스를 재시작합니다.
         */
        void ReloadChangedScripts();

        /**
         * @brief 이번 프레임에 Lua Input 바인딩이 읽을 입력을 지정합니다.
         * @param[in] Input 복사해 둘 이번 프레임 입력입니다.
         */
        void SetFrameInput(const Core::FInput& Input);

        /**
         * @brief Lua Input 바인딩이 읽는 입력을 비워 입력이 없는 상태로 만듭니다.
         */
        void ClearFrameInput();

    private:
        /**
         * @brief 수학 타입, 충돌 결과, Actor, Component, World, Input 등 엔진 API를 Lua에 등록합니다.
         */
        void RegisterBindings(sol::state_view Lua);

        Engine::UActorComponent* FindComponent(const Engine::AActor& Actor, const CoreUObject::UClass* Class) const;
        Engine::UProjectileMovementComponent& ResolveMovement(const Engine::AActor& Actor) const;

        /**
         * @brief 스크립트 파일을 읽어 UTF-8 BOM을 제거하고 텍스트 청크로 컴파일합니다.
         * @param[in] Path 읽을 스크립트 파일 경로입니다.
         * @return 컴파일한 청크이며 실패하면 오류를 보고하고 유효하지 않은 함수를 반환합니다.
         */
        sol::protected_function LoadChunk(const std::filesystem::path& Path);

        /**
         * @brief 새 전역 환경에서 청크를 실행하고 정의된 수명 주기 콜백을 수집합니다.
         * @param[in] Instance 환경에 Actor로 노출할 소유 액터와 오류 보고 정보를 제공하는 인스턴스입니다.
         * @param[in] Chunk 실행할 컴파일된 청크입니다.
         * @param[out] Execution 만든 환경과 콜백을 기록할 대상입니다.
         * @return 청크가 유효하고 초기화 중 오류가 없었으면 true입니다.
         * @note 청크가 함수를 반환하면 지원하지 않는 형식으로 보고 실패합니다.
         */
        bool CreateExecution(const Engine::FLuaScriptInstance& Instance, const sol::protected_function& Chunk, Script::FLuaExecution& Execution);

        /**
         * @brief 파일을 다시 컴파일하고 그 파일을 쓰는 모든 인스턴스의 실행을 교체합니다.
         * @param[in] Path 다시 불러올 스크립트 파일 경로입니다.
         * @param[in,out] Source 갱신할 파일 항목입니다.
         * @note 컴파일이나 어느 한 인스턴스의 초기화라도 실패하면 기존 실행을 그대로 유지합니다. 교체할 때는 기존 EndPlay와 새 BeginPlay를 차례로 호출합니다.
         */
        void Reload(const std::filesystem::path& Path, FSource& Source);

        /**
         * @brief 스크립트 경로, 이벤트와 액터 이름을 붙여 Lua 경고를 보고합니다.
         * @param[in] Path 문제가 생긴 스크립트 파일 경로입니다.
         * @param[in] Owner 스크립트를 실행한 액터이며 없으면 nullptr입니다.
         * @param[in] Event 문제가 생긴 단계나 콜백 이름입니다.
         * @param[in] Message 오류 내용입니다.
         */
        static void Report(const std::filesystem::path& Path, const Engine::AActor* Owner, const char* Event, std::string_view Message);

    private:
        /**
         * @brief 런타임을 소유하며 World 바인딩이 조회하는 월드입니다.
         */
        Engine::UWorld& mWorld;

        /**
         * @brief Lua Input 바인딩이 읽는 이번 프레임 입력입니다.
         */
        Core::FInput mFrameInput{};

        /**
         * @brief 모든 스크립트가 공유하는 Lua 상태입니다.
         */
        Script::FLuaVM mLuaVM{};

        /**
         * @brief 스크립트 파일 경로별 컴파일 결과입니다.
         */
        std::map<std::filesystem::path, FSource> mSources{};

        /**
         * @brief 컴포넌트별 실행 인스턴스이며 주소가 바뀌지 않도록 map에 보관합니다.
         */
        std::map<Engine::ULuaScriptComponent*, Engine::FLuaScriptInstance> mInstances{};

        /**
         * @brief 사용 중인 스크립트 파일의 디스크 변경을 감시합니다.
         */
        Core::FPathTracker mPathTracker{};
    };

}
