/**
 * @file FParticleEmitterInstance.h
 * @brief Emitter 원본 참조와 인스턴스별 typeless 모듈 페이로드의 수명을 선언합니다.
 * @ingroup Cascade
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/TObjectRef.h"

#include <memory>

/**
 * @brief 파티클 시스템 정의와 모듈 실행 컨텍스트 및 인스턴스 저장소를 제공합니다.
 */
namespace Cascade {

    class UParticleEmitter;
    class UParticleModule;
    class IParticleEmitterInstanceOwner;

    /**
     * @brief Emitter 정의를 참조하며 인스턴스별 모듈 페이로드를 소유하는 실행 객체입니다.
     * @details Init()에서 모듈 목록과 배치를 복사하고 추가 메모리를 초기화합니다. 현재 구현에는 자동 파티클 생성·갱신 루프가 없습니다.
     * @ingroup Cascade
     */
    class FParticleEmitterInstance {
    private:
        /**
         * @brief 16바이트 정렬로 할당한 인스턴스 페이로드를 해제하는 삭제자입니다.
         */
        struct FPayloadDeleter {
        public:
            /**
             * @brief 정렬 할당과 짝을 이루는 연산자로 페이로드 저장 공간을 해제합니다.
             * @param[in] Data 16바이트 정렬로 할당한 버퍼의 시작 주소입니다.
             */
            void operator()(Core::Uint8* Data) const noexcept;
        };

    public:
        /**
         * @brief 소유자 인터페이스의 비소유 참조를 보관하는 인스턴스를 구성합니다.
         * @param[in] Owner 인스턴스보다 오래 유지되어야 하는 소유자 객체입니다.
         */
        explicit FParticleEmitterInstance(Cascade::IParticleEmitterInstanceOwner& Owner);
        /**
         * @brief 인스턴스 페이로드와 모듈 정의에 대한 소유 참조를 해제합니다.
         */
        virtual ~FParticleEmitterInstance();

    public:
        /**
         * @brief 원본 Emitter 참조를 교체하고 기존 페이로드와 배치 스냅샷을 비웁니다.
         * @param[in] Template 원본 Emitter의 비소유 주소이며 nullptr이면 참조를 해제합니다.
         * @note 이 함수 뒤에 Init()을 호출해 새 실행 메모리를 준비합니다.
         */
        virtual void InitParameters(Cascade::UParticleEmitter* Template);
        /**
         * @brief 원본 정의의 배치를 복사하고 모듈별 인스턴스 페이로드를 초기화합니다.
         * @details 버퍼를 16바이트 정렬로 할당하고 0으로 채운 뒤, 데이터가 필요한 모듈의 PrepPerInstanceBlock()을 호출합니다. 원본이 없으면 아무 작업도 하지 않습니다.
         * @note 다시 호출하면 기존 실행 페이로드가 초기화됩니다.
         */
        virtual void Init();

        /**
         * @brief 공간 변환과 인스턴스 이름을 제공하는 소유자를 반환합니다.
         * @return 생성 시 전달한 소유자 인터페이스의 비소유 참조입니다.
         */
        Cascade::IParticleEmitterInstanceOwner& GetOwner() const;
        /**
         * @brief 세대 핸들로 원본 Emitter 참조를 해석합니다.
         * @return 원본 정의의 비소유 주소이며 파괴되었거나 연결하지 않았으면 nullptr입니다.
         */
        Cascade::UParticleEmitter* GetTemplate() const;
        /**
         * @brief 배치 스냅샷에서 모듈의 파티클 추가 데이터 오프셋을 조회합니다.
         * @param[in] Module 조회할 모듈 정의의 비소유 주소입니다.
         * @return 파티클 레코드 내 바이트 오프셋이며 추가 영역이 없으면 0입니다.
         */
        Core::Index_t GetModuleDataOffset(const Cascade::UParticleModule* Module) const;
        /**
         * @brief 이 인스턴스에서 모듈에 배정한 실행 상태 영역을 반환합니다.
         * @param[in] Module 실행 상태 영역을 조회할 모듈의 비소유 주소입니다.
         * @return 수정 가능한 데이터 주소이며 배정한 영역이 없으면 nullptr입니다.
         * @note 반환 주소는 재초기화 또는 인스턴스 소멸 전까지 유효합니다.
         */
        Core::Uint8* GetModuleInstanceData(const Cascade::UParticleModule* Module);
        /**
         * @brief 이 인스턴스에서 모듈에 배정한 실행 상태 영역을 읽기 전용으로 반환합니다.
         * @param[in] Module 실행 상태 영역을 조회할 모듈의 비소유 주소입니다.
         * @return 읽기 전용 데이터 주소이며 배정한 영역이 없으면 nullptr입니다.
         * @note 반환 주소는 재초기화 또는 인스턴스 소멸 전까지 유효합니다.
         */
        const Core::Uint8* GetModuleInstanceData(const Cascade::UParticleModule* Module) const;
        /**
         * @brief 이 인스턴스에 준비한 모듈 실행 데이터 버퍼의 크기를 반환합니다.
         * @return 정렬 패딩을 포함한 바이트 수입니다.
         */
        Core::Uint32 GetInstancePayloadSize() const;

    private:
        /**
         * @brief 인스턴스보다 오래 유지할 소유자 인터페이스의 비소유 참조입니다.
         */
        Cascade::IParticleEmitterInstanceOwner& mOwner;
        /**
         * @brief 원본 Emitter의 파괴를 감지할 수 있는 런타임 세대 핸들입니다.
         */
        CoreUObject::TObjectRef<Cascade::UParticleEmitter> mTemplate{};
        /**
         * @brief 배치 스냅샷에서 사용하는 모듈 정의의 수명을 유지하는 공유 참조 목록입니다.
         */
        Core::TArray<std::shared_ptr<Cascade::UParticleModule>> mModules{};
        /**
         * @brief 초기화 시 복사한 모듈별 파티클 추가 데이터의 바이트 오프셋입니다.
         */
        Core::TMap<const Cascade::UParticleModule*, Core::Index_t> mModuleDataOffsets{};
        /**
         * @brief 초기화 시 복사한 모듈별 인스턴스 실행 데이터의 바이트 오프셋입니다.
         */
        Core::TMap<const Cascade::UParticleModule*, Core::Index_t> mModuleInstanceDataOffsets{};
        /**
         * @brief 이 인스턴스가 단독 소유하며 정렬 삭제자로 해제하는 typeless 실행 데이터 버퍼입니다.
         */
        std::unique_ptr<Core::Uint8, Cascade::FParticleEmitterInstance::FPayloadDeleter> mInstanceData{};
        /**
         * @brief 실제로 준비한 인스턴스 실행 데이터와 정렬 패딩의 전체 바이트 수입니다.
         */
        Core::Uint32 mInstancePayloadSize{};
    };

}
