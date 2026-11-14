/**
 * @file UParticleModule.h
 * @brief 모듈의 생성·갱신 컨텍스트와 추가 메모리 요구량 및 실행 단계 설정을 선언합니다.
 * @ingroup Cascade
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/UObject.h"

#include <cstddef>

#include "UParticleModule.generated.h"

namespace Core {
    class FArchive;
    struct FTransform;
}

/**
 * @brief 파티클 시스템 정의와 모듈 실행 컨텍스트 및 인스턴스 저장소를 제공합니다.
 */
namespace Engine {

    class FParticleEmitterInstance;
    class UParticleLODLevel;
    class UParticleModuleTypeDataBase;
    struct FBaseParticle;

    /**
     * @brief 파티클의 생성·갱신 규칙과 추가 페이로드 요구량을 정의하는 UObject입니다.
     * @details 설정은 여러 인스턴스가 공유합니다. 실행 중 변하는 상태는 모듈 멤버 대신 각 EmitterInstance의 페이로드에 저장합니다.
     * @ingroup Cascade
     */
    UCLASS()
    class UParticleModule : public CoreUObject::UObject {
        GENERATED_BODY()

    private:
        friend class Engine::UParticleLODLevel;

    public:
        /**
         * @brief 모듈 호출에서 실행 대상 EmitterInstance를 참조하는 공통 컨텍스트입니다.
         * @note 컨텍스트는 인스턴스를 소유하지 않으며 호출 중 인스턴스가 살아 있어야 합니다.
         */
        struct FContext {
        public:
            /**
             * @brief 실행 인스턴스에 대한 비소유 참조로 공통 컨텍스트를 구성합니다.
             * @param[in] Owner 호출 동안 살아 있어야 하는 실행 인스턴스입니다.
             */
            explicit FContext(Engine::FParticleEmitterInstance& Owner);

        public:
            /**
             * @brief 이 호출의 실행 상태를 보관하는 인스턴스를 반환합니다.
             * @return 컨텍스트가 참조하는 인스턴스의 수정 가능한 참조입니다.
             */
            Engine::FParticleEmitterInstance& GetOwner() const;
            /**
             * @brief 소유자가 제공하는 현재 공간 변환을 반환합니다.
             * @return 소유자 객체가 유지하는 변환의 읽기 전용 참조입니다.
             */
            const Core::FTransform& GetTransform() const;
            /**
             * @brief 실행 소유자가 제공하는 인스턴스 이름을 반환합니다.
             * @return 소유자 인터페이스에서 얻은 이름입니다.
             */
            Core::FString GetInstanceName() const;
            /**
             * @brief 인스턴스의 원본 Emitter 정의 이름을 반환합니다.
             * @return 살아 있는 원본의 이름이며 원본 참조가 해제되었으면 빈 문자열입니다.
             */
            Core::FString GetTemplateName() const;

        private:
            /**
             * @brief 모듈 호출이 사용할 실행 인스턴스의 비소유 참조입니다.
             */
            Engine::FParticleEmitterInstance& mOwner;
        };

        /**
         * @brief 새 파티클 하나를 초기화할 때 대상 파티클과 시간 및 페이로드 오프셋을 전달합니다.
         */
        struct FSpawnContext : public Engine::UParticleModule::FContext {
        public:
            /**
             * @brief 새 파티클 초기화 호출에 필요한 정보를 구성합니다.
             * @param[in] Owner 실행 인스턴스입니다.
             * @param[in] Offset 파티클 레코드 시작에서 모듈 추가 데이터까지의 바이트 오프셋입니다.
             * @param[in] SpawnTime 생성 시점에 전달할 시간 값입니다.
             * @param[in] ParticleBase 초기화할 기본 파티클 데이터의 비소유 주소입니다.
             */
            FSpawnContext(Engine::FParticleEmitterInstance& Owner, Core::Index_t Offset, float SpawnTime, Engine::FBaseParticle* ParticleBase);

        public:
            /**
             * @brief 파티클 레코드 내 모듈 추가 데이터의 위치를 반환합니다.
             * @return 레코드 시작을 기준으로 한 바이트 오프셋입니다.
             */
            Core::Index_t GetOffset() const;
            /**
             * @brief 이번 생성 호출에 전달된 시간 값을 반환합니다.
             * @return 컨텍스트 생성 시 지정한 SpawnTime입니다.
             */
            float GetSpawnTime() const;
            /**
             * @brief 이번 호출에서 초기화할 기본 파티클 데이터를 반환합니다.
             * @return 호출자가 제공한 파티클의 비소유 주소입니다.
             */
            Engine::FBaseParticle* GetParticleBase() const;

        private:
            /**
             * @brief 파티클 레코드 시작에서 이 모듈의 추가 데이터까지의 바이트 오프셋입니다.
             */
            Core::Index_t mOffset{};
            /**
             * @brief 새 파티클의 초기화 호출에 전달할 생성 시간 값입니다.
             */
            float mSpawnTime{};
            /**
             * @brief 이번 호출에서 초기화할 기본 파티클 데이터의 비소유 주소입니다.
             */
            Engine::FBaseParticle* mParticleBase{};
        };

        /**
         * @brief 모듈 갱신에 사용할 인스턴스와 파티클 페이로드 오프셋 및 시간 간격을 전달합니다.
         */
        struct FUpdateContext : public Engine::UParticleModule::FContext {
        public:
            /**
             * @brief 모듈 갱신 호출의 인스턴스와 데이터 위치 및 시간 간격을 구성합니다.
             * @param[in] Owner 실행 인스턴스입니다.
             * @param[in] Offset 파티클 레코드 내 모듈 추가 데이터의 바이트 오프셋입니다.
             * @param[in] DeltaTime 이번 갱신에 사용할 경과 시간입니다.
             */
            FUpdateContext(Engine::FParticleEmitterInstance& Owner, Core::Index_t Offset, float DeltaTime);

        public:
            /**
             * @brief 파티클 레코드 내 모듈 추가 데이터의 위치를 반환합니다.
             * @return 레코드 시작을 기준으로 한 바이트 오프셋입니다.
             */
            Core::Index_t GetOffset() const;
            /**
             * @brief 이번 모듈 갱신에 전달된 시간 간격을 반환합니다.
             * @return 컨텍스트 생성 시 지정한 DeltaTime입니다.
             */
            float GetDeltaTime() const;

        private:
            /**
             * @brief 파티클 레코드 시작에서 이 모듈의 추가 데이터까지의 바이트 오프셋입니다.
             */
            Core::Index_t mOffset{};
            /**
             * @brief 이번 모듈 갱신에 전달할 경과 시간입니다.
             */
            float mDeltaTime{};
        };

    public:
        /**
         * @brief 기본 모듈 이름과 활성·실행 단계 설정을 구성합니다.
         */
        UParticleModule();
        /**
         * @brief 모듈 UObject의 수명을 종료합니다. 인스턴스 실행 메모리는 소유하지 않습니다.
         */
        ~UParticleModule() override;

    public:
        /**
         * @brief UObject 식별 정보와 Serialize 프로퍼티로 등록된 모듈 설정을 직렬화합니다.
         * @param[in,out] Archive 로드 또는 저장 모드의 아카이브입니다.
         * @note 인스턴스별 실행 상태는 직렬화 대상에 포함하지 않습니다.
         */
        void Serialize(Core::FArchive& Archive) override;

        /**
         * @brief 이 모듈이 파티클 하나마다 요구하는 추가 데이터 크기를 반환합니다.
         * @param[in] TypeData Emitter 유형별 데이터 정의의 비소유 주소이며 현재 호출 경로에서는 nullptr입니다.
         * @return FBaseParticle을 제외한 추가 바이트 수이며 기본 구현은 0입니다.
         */
        virtual Core::Uint32 RequiredBytes(Engine::UParticleModuleTypeDataBase* TypeData);
        /**
         * @brief 이 모듈이 EmitterInstance 하나마다 요구하는 추가 데이터 크기를 반환합니다.
         * @return 파티클 개수와 무관하게 인스턴스당 한 번 할당할 바이트 수이며 기본 구현은 0입니다.
         */
        virtual Core::Uint32 RequiredBytesPerInstance();
        /**
         * @brief 모듈에 할당된 인스턴스 페이로드를 초기화하는 확장 지점입니다.
         * @param[in] Owner 페이로드를 소유하는 실행 인스턴스의 비소유 주소입니다.
         * @param[in,out] InstanceData 이 모듈에 배정된 데이터 영역의 시작 주소입니다.
         * @return 기본 구현은 추가 작업 없이 0을 반환합니다.
         * @note 실행 중 변하는 필드는 모듈 멤버가 아닌 이 영역에서 관리합니다.
         */
        virtual Core::Uint32 PrepPerInstanceBlock(Engine::FParticleEmitterInstance* Owner, void* InstanceData);

        /**
         * @brief 생성된 파티클 하나의 초기 속성을 설정하는 확장 지점입니다.
         * @param[in] Context 실행 인스턴스와 새 파티클·생성 시간·추가 데이터 오프셋을 담은 컨텍스트입니다.
         * @note 기본 구현은 아무 작업도 하지 않습니다. 생성할 파티클 개수를 결정하는 함수가 아닙니다.
         */
        virtual void Spawn(const Engine::UParticleModule::FSpawnContext& Context);
        /**
         * @brief 인스턴스의 대상 파티클들을 모듈 단위로 갱신하는 확장 지점입니다.
         * @param[in] Context 실행 인스턴스와 모듈 데이터 오프셋 및 경과 시간을 담은 컨텍스트입니다.
         * @note 기본 구현은 아무 작업도 하지 않습니다.
         */
        virtual void Update(const Engine::UParticleModule::FUpdateContext& Context);
        /**
         * @brief 일반 갱신 뒤에 필요한 파티클 후처리를 수행하는 확장 지점입니다.
         * @param[in] Context 실행 인스턴스와 모듈 데이터 오프셋 및 경과 시간을 담은 컨텍스트입니다.
         * @note 기본 구현은 아무 작업도 하지 않습니다.
         */
        virtual void FinalUpdate(const Engine::UParticleModule::FUpdateContext& Context);

    private:
        void SerializeModule(Core::FArchive& Archive);

    public:
        /**
         * @brief 실행 단계 목록에 이 모듈을 포함할지 결정하는 공유 설정입니다.
         */
        UPROPERTY(Edit, Serialize)
        bool mEnabled{true};

        /**
         * @brief 활성 상태에서 새 파티클 초기화 단계에 참여할지 결정하는 설정입니다.
         */
        UPROPERTY(Edit, Serialize)
        bool mSpawnModule{};

        /**
         * @brief 활성 상태에서 일반 파티클 갱신 단계에 참여할지 결정하는 설정입니다.
         */
        UPROPERTY(Edit, Serialize)
        bool mUpdateModule{};

        /**
         * @brief 활성 상태에서 최종 파티클 갱신 단계에 참여할지 결정하는 설정입니다.
         */
        UPROPERTY(Edit, Serialize)
        bool mFinalUpdateModule{};

    };

}
