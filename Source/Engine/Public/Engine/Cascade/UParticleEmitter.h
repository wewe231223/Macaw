/**
 * @file UParticleEmitter.h
 * @brief LOD별 모듈 정의와 파티클·인스턴스 페이로드 배치를 소유하는 이미터입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/UObject.h"
#include "Engine/Cascade/FParticleEmitterInstance.h"
#include "Core/Archive.h"
#include "Engine/Cascade/IParticleEmitterInstanceOwner.h"
#include "Engine/Cascade/UParticleLODLevel.h"
#include "Engine/Cascade/UParticleModule.h"

#include <cstddef>
#include <memory>

#include "UParticleEmitter.generated.h"

namespace Engine {

    /**
     * @brief LOD별 모듈 정의와 파티클·인스턴스 페이로드 배치를 소유하는 이미터입니다.
     */
    UCLASS()
    class UParticleEmitter : public CoreUObject::UObject {
        GENERATED_BODY()

    private:
        /**
         * @brief UParticleLODLevel에서 내부 연결 또는 저장 상태에 접근하도록 허용합니다.
         */
        friend class Engine::UParticleLODLevel;

    public:
        /**
         * @brief UParticleEmitter의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        UParticleEmitter();

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~UParticleEmitter() override;

    public:
        /**
         * @brief 소유 가능한 LOD 정의를 이미터에 추가하고 빌드 데이터를 무효화합니다.
         * @param[in] LODLevel 이미터에 소유권을 이전할 LOD 정의입니다.
         * @return LOD를 추가했으면 true입니다.
         */
        bool AddLODLevel(std::unique_ptr<Engine::UParticleLODLevel> LODLevel);

        /**
         * @brief 지정한 LOD 정의를 찾아 제거하고 빌드 데이터를 무효화합니다.
         * @param[in] LODLevel 제거할 LOD 정의의 비소유 참조입니다.
         * @return 대상 LOD를 제거했으면 true입니다.
         */
        bool RemoveLODLevel(CoreUObject::TObjectRef<const Engine::UParticleLODLevel> LODLevel);

        /**
         * @brief 모든 LOD 정의를 제거하고 캐시된 페이로드 배치를 비웁니다.
         */
        void ClearLODLevels();

        /**
         * @brief 인덱스로 LOD 정의의 형식 지정 참조를 조회합니다.
         * @param[in] Index 조회하거나 변경할 0부터 시작하는 인덱스입니다.
         * @return LOD 비소유 참조이며 범위를 벗어나면 빈 참조입니다.
         */
        CoreUObject::TObjectRef<Engine::UParticleLODLevel> GetLODLevel(Index_t Index);

        /**
         * @brief 인덱스로 LOD 정의의 형식 지정 참조를 조회합니다.
         * @param[in] Index 조회하거나 변경할 0부터 시작하는 인덱스입니다.
         * @return LOD 비소유 참조이며 범위를 벗어나면 빈 참조입니다.
         */
        CoreUObject::TObjectRef<const Engine::UParticleLODLevel> GetLODLevel(Index_t Index) const;

        /**
         * @brief 이미터가 소유하는 LOD 정의 수를 반환합니다.
         * @return 현재 LOD 수입니다.
         */
        std::size_t GetLODLevelCount() const;

        /**
         * @brief 이미터가 소유하는 LOD 정의 목록을 반환합니다.
         * @return LOD 소유 컨테이너의 읽기 전용 참조입니다.
         */
        const Core::TArray<std::unique_ptr<Engine::UParticleLODLevel>>& GetLODLevels() const;

        /**
         * @brief 모든 LOD에서 사용하는 중복 없는 모듈 참조를 수집합니다.
         * @return 필수 모듈을 포함한 전체 모듈 참조 목록입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> GetAllModules() const;

        /**
         * @brief LOD 모듈 실행 목록과 페이로드 배치 캐시를 다시 구성합니다.
         */
        void Build();

        /**
         * @brief 모든 LOD 모듈의 파티클·인스턴스 영역 크기와 정렬 오프셋을 계산합니다.
         */
        void CacheEmitterModuleInfo();

        /**
         * @brief 지정한 소유자에 연결된 실행 인스턴스를 만들고 정의와 실행 메모리를 초기화합니다.
         * @param[in] Owner 실행 인스턴스보다 오래 유지하며 변환과 이름을 제공할 소유자입니다.
         * @return 새 실행 인스턴스의 소유권입니다.
         */
        virtual std::unique_ptr<Engine::FParticleEmitterInstance> CreateInstance(Engine::IParticleEmitterInstanceOwner& Owner);

        /**
         * @brief 모듈별 추가 영역을 포함한 파티클 레코드 크기를 반환합니다.
         * @return 정렬 패딩을 포함한 바이트 수입니다.
         */
        Uint32 GetParticleSize() const;

        /**
         * @brief 이미터 인스턴스별 모듈 데이터의 전체 크기를 반환합니다.
         * @return 정렬 패딩을 포함한 바이트 수입니다.
         */
        Uint32 GetInstancePayloadSize() const;

        /**
         * @brief 모듈별 파티클 추가 데이터의 배치 오프셋을 반환합니다.
         * @return 모듈 참조에서 바이트 오프셋으로의 읽기 전용 맵입니다.
         */
        const CoreUObject::ObjectHashMap<CoreUObject::TObjectRef<const Engine::UParticleModule>, Index_t>& GetModuleDataOffsets() const;

        /**
         * @brief 모듈별 인스턴스 데이터의 배치 오프셋을 반환합니다.
         * @return 모듈 참조에서 바이트 오프셋으로의 읽기 전용 맵입니다.
         */
        const CoreUObject::ObjectHashMap<CoreUObject::TObjectRef<const Engine::UParticleModule>, Index_t>& GetModuleInstanceDataOffsets() const;

        void Serialize(Core::FArchive& Archive) override;

    private:
        /**
         * @brief 모듈 구성 변경으로 무효해진 페이로드 배치 캐시를 초기화합니다.
         */
        void InvalidateBuildData();

    private:
        /**
         * @brief 이미터가 소유하는 LOD 정의 목록입니다.
         */
        Core::TArray<std::unique_ptr<Engine::UParticleLODLevel>> mLODLevels{};

        /**
         * @brief 모듈별 파티클 추가 데이터의 바이트 오프셋 맵입니다.
         */
        CoreUObject::ObjectHashMap<CoreUObject::TObjectRef<const Engine::UParticleModule>, Index_t> mModuleDataOffsets{};

        /**
         * @brief 모듈별 인스턴스 데이터의 바이트 오프셋 맵입니다.
         */
        CoreUObject::ObjectHashMap<CoreUObject::TObjectRef<const Engine::UParticleModule>, Index_t> mModuleInstanceDataOffsets{};

        /**
         * @brief 기본 파티클과 모듈 추가 영역을 포함한 정렬 레코드 크기입니다.
         */
        Uint32 mParticleSize{};

        /**
         * @brief 정렬 패딩을 포함한 이미터 인스턴스 모듈 데이터 크기입니다.
         */
        Uint32 mInstancePayloadSize{};
    };

}
