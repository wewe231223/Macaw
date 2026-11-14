/**
 * @file UParticleEmitter.h
 * @brief Emitter의 모듈 구성·실행 단계 분류·페이로드 배치와 인스턴스 생성을 선언합니다.
 * @ingroup Cascade
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/UObject.h"
#include "Cascade/FParticleEmitterInstance.h"

#include <cstddef>
#include <memory>

#include "UParticleEmitter.generated.h"

namespace Core {
    class FArchive;
}

/**
 * @brief 파티클 시스템 정의와 모듈 실행 컨텍스트 및 인스턴스 저장소를 제공합니다.
 */
namespace Cascade {

    class UParticleModule;
    class IParticleEmitterInstanceOwner;

    /**
     * @brief 공유할 모듈 설정과 실행 단계별 목록 및 페이로드 배치를 보관하는 Emitter 정의입니다.
     * @details 모듈 설정을 변경한 뒤에는 Build()로 실행 준비 정보를 다시 구성합니다. 실제 실행 메모리는 FParticleEmitterInstance가 소유합니다.
     * @ingroup Cascade
     */
    UCLASS()
    class UParticleEmitter : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        /**
         * @brief UObject 이름을 Emitter로 설정하고 빈 모듈 정의를 구성합니다.
         */
        UParticleEmitter();
        /**
         * @brief 모듈의 Outer 연결과 Emitter가 가진 소유 참조 및 배치 캐시를 해제합니다.
         */
        ~UParticleEmitter() override;

    public:
        /**
         * @brief 모듈을 자식으로 추가하고 실행 단계 목록을 갱신합니다.
         * @param[in] Module Emitter로 소유권을 이전할 모듈입니다.
         * @return 추가하면 true이며 null·다른 Outer·개수 제한 등으로 거부하면 false입니다.
         * @note 페이로드 배치 캐시는 무효화되므로 사용 전에 Build()를 호출합니다.
         */
        bool AddModule(std::unique_ptr<Cascade::UParticleModule> Module);
        /**
         * @brief 모듈의 자식 연결을 제거하고 실행 단계 목록과 배치 캐시를 갱신합니다.
         * @param[in] Module 제거할 모듈의 비소유 참조입니다.
         * @return 제거했으면 true이며 대상이 없으면 false입니다.
         */
        bool RemoveModule(CoreUObject::TObjectRef<const Cascade::UParticleModule> Module);
        /**
         * @brief 모든 모듈의 Outer 연결과 소유 목록 및 실행 준비 정보를 비웁니다.
         */
        void ClearModules();

        /**
         * @brief 인덱스에 해당하는 모듈의 수정 가능한 비소유 참조를 반환합니다.
         * @param[in] Index 모듈 목록의 0부터 시작하는 인덱스입니다.
         * @return 모듈의 참조이며 범위를 벗어나면 빈 참조입니다.
         */
        CoreUObject::TObjectRef<Cascade::UParticleModule> GetModule(Core::Index_t Index);
        /**
         * @brief 인덱스에 해당하는 모듈의 읽기 전용 비소유 참조를 반환합니다.
         * @param[in] Index 모듈 목록의 0부터 시작하는 인덱스입니다.
         * @return 모듈의 참조이며 범위를 벗어나면 빈 참조입니다.
         */
        CoreUObject::TObjectRef<const Cascade::UParticleModule> GetModule(Core::Index_t Index) const;
        /**
         * @brief Emitter에 등록된 모듈 정의의 개수를 반환합니다.
         * @return 비활성 모듈도 포함한 원소 수입니다.
         */
        std::size_t GetModuleCount() const;
        /**
         * @brief 설정 순서를 유지하는 전체 모듈의 비소유 참조를 제공합니다.
         * @return Emitter가 소유하는 모듈들의 비소유 참조 스냅샷입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Cascade::UParticleModule>> GetModules() const;

        /**
         * @brief 실행 단계별 모듈 목록과 파티클·인스턴스 페이로드 배치를 다시 구성합니다.
         * @note 시뮬레이션을 실행하거나 인스턴스별 메모리를 할당하지 않습니다.
         */
        void Build();
        /**
         * @brief 모듈의 요구 바이트 수로 16바이트 정렬 오프셋과 전체 레코드 크기를 계산합니다.
         * @details 파티클 크기는 FBaseParticle을 포함하며 인스턴스 크기는 모듈별 추가 영역의 합입니다. 현재 TypeData 인자는 nullptr로 전달합니다.
         * @throws std::length_error 페이로드가 지원하는 오프셋 범위를 초과한 경우입니다.
         */
        void CacheEmitterModuleInfo();
        /**
         * @brief 활성 모듈을 Spawn·Update·FinalUpdate 참여 플래그에 따라 순서대로 분류합니다.
         * @note 공개 플래그를 직접 수정한 뒤에는 이 함수 또는 Build()로 목록을 갱신합니다.
         */
        virtual void UpdateModuleLists();
        /**
         * @brief 이 정의를 참조하는 인스턴스를 생성하고 모듈 페이로드를 초기화합니다.
         * @param[in] Owner 인스턴스보다 오래 유지할 소유자 인터페이스입니다.
         * @return 새 인스턴스의 단독 소유권을 가진 unique_ptr입니다.
         */
        virtual std::unique_ptr<Cascade::FParticleEmitterInstance> CreateInstance(Cascade::IParticleEmitterInstanceOwner& Owner);

        /**
         * @brief 새 파티클 초기화 단계에 참여하는 활성 모듈 목록을 반환합니다.
         * @return 모듈 소유 목록의 순서를 유지하는 비소유 참조 목록의 참조입니다.
         */
        const Core::TArray<CoreUObject::TObjectRef<Cascade::UParticleModule>>& GetSpawnModules() const;
        /**
         * @brief 파티클 갱신 단계에 참여하는 활성 모듈 목록을 반환합니다.
         * @return 모듈 소유 목록의 순서를 유지하는 비소유 참조 목록의 참조입니다.
         */
        const Core::TArray<CoreUObject::TObjectRef<Cascade::UParticleModule>>& GetUpdateModules() const;
        /**
         * @brief 최종 갱신 단계에 참여하는 활성 모듈 목록을 반환합니다.
         * @return 모듈 소유 목록의 순서를 유지하는 비소유 참조 목록의 참조입니다.
         */
        const Core::TArray<CoreUObject::TObjectRef<Cascade::UParticleModule>>& GetFinalUpdateModules() const;

        /**
         * @brief 기본 파티클과 모듈 추가 데이터 및 패딩을 합친 레코드 크기를 반환합니다.
         * @return Build()에서 계산한 16바이트 정렬 크기이며 캐시가 무효화된 상태이면 0입니다.
         */
        Core::Uint32 GetParticleSize() const;
        /**
         * @brief 인스턴스 하나에 필요한 전체 모듈 페이로드 크기를 반환합니다.
         * @return 정렬 패딩을 포함한 바이트 수입니다.
         */
        Core::Uint32 GetInstancePayloadSize() const;
        /**
         * @brief 모듈별 파티클 추가 데이터의 시작 오프셋 대응표를 반환합니다.
         * @return 파티클 레코드 시작을 기준으로 한 바이트 오프셋 목록의 참조입니다.
         * @note 파티클 추가 데이터를 요구하지 않는 모듈은 포함하지 않습니다.
         */
        const Core::TMap<CoreUObject::TObjectRef<const Cascade::UParticleModule>, Core::Index_t, CoreUObject::TObjectRefHash<const Cascade::UParticleModule>>& GetModuleDataOffsets() const;
        /**
         * @brief 모듈별 인스턴스 추가 데이터의 시작 오프셋 대응표를 반환합니다.
         * @return 인스턴스 버퍼 시작을 기준으로 한 바이트 오프셋 목록의 참조입니다.
         * @note 인스턴스 데이터를 요구하지 않는 모듈은 포함하지 않습니다.
         */
        const Core::TMap<CoreUObject::TObjectRef<const Cascade::UParticleModule>, Core::Index_t, CoreUObject::TObjectRefHash<const Cascade::UParticleModule>>& GetModuleInstanceDataOffsets() const;

        /**
         * @brief Emitter 이름과 모듈 클래스·설정을 아카이브로 읽거나 기록합니다.
         * @param[in,out] Archive 로드 또는 저장 모드의 아카이브입니다.
         * @details 로드 중 모듈은 UClass 팩토리로 복원하며 배치 준비가 끝난 후보의 설정만 반영합니다. 캐시와 실행 페이로드는 저장하지 않습니다.
         */
        void Serialize(Core::FArchive& Archive) override;

    private:
        /**
         * @brief 모듈 구성이 바뀌어 유효하지 않은 오프셋과 레코드 크기 캐시를 비웁니다.
         */
        void InvalidateBuildData();

    private:
        /**
         * @brief 설정 순서대로 자식 모듈 UObject를 단독 소유하는 목록입니다.
         */
        Core::TArray<std::unique_ptr<Cascade::UParticleModule>> mModules{};
        /**
         * @brief Spawn에 참여하는 활성 모듈의 비소유 참조 캐시입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Cascade::UParticleModule>> mSpawnModules{};
        /**
         * @brief Update에 참여하는 활성 모듈의 비소유 참조 캐시입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Cascade::UParticleModule>> mUpdateModules{};
        /**
         * @brief FinalUpdate에 참여하는 활성 모듈의 비소유 참조 캐시입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Cascade::UParticleModule>> mFinalUpdateModules{};
        /**
         * @brief 모듈별 파티클 추가 데이터의 레코드 내 바이트 오프셋입니다.
         */
        Core::TMap<CoreUObject::TObjectRef<const Cascade::UParticleModule>, Core::Index_t, CoreUObject::TObjectRefHash<const Cascade::UParticleModule>> mModuleDataOffsets{};
        /**
         * @brief 모듈별 인스턴스 페이로드의 버퍼 내 바이트 오프셋입니다.
         */
        Core::TMap<CoreUObject::TObjectRef<const Cascade::UParticleModule>, Core::Index_t, CoreUObject::TObjectRefHash<const Cascade::UParticleModule>> mModuleInstanceDataOffsets{};
        /**
         * @brief 기본 파티클과 추가 데이터 및 정렬 패딩을 포함한 레코드 크기입니다.
         */
        Core::Uint32 mParticleSize{};
        /**
         * @brief 인스턴스 하나에 필요한 모듈 페이로드와 정렬 패딩의 전체 바이트 수입니다.
         */
        Core::Uint32 mInstancePayloadSize{};
    };

}
