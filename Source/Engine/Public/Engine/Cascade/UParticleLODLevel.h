/**
 * @file UParticleLODLevel.h
 * @brief 하나의 LOD에서 필수 모듈과 실행 단계별 파티클 모듈을 소유합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/UObject.h"
#include "Core/Archive.h"
#include "Engine/Cascade/UParticleModule.h"
#include "Engine/Cascade/UParticleModuleRequired.h"

#include <cstddef>
#include <memory>

#include "UParticleLODLevel.generated.h"

namespace Engine {

    /**
     * @brief 하나의 LOD에서 필수 모듈과 실행 단계별 파티클 모듈을 소유합니다.
     */
    UCLASS()
    class UParticleLODLevel : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        /**
         * @brief UParticleLODLevel의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        UParticleLODLevel();

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~UParticleLODLevel() override;

    public:
        /**
         * @brief 해당 파티클 LOD의 활성 여부를 반환합니다.
         * @return LOD가 활성화되어 있으면 true입니다.
         */
        bool GetEnabled() const;

        /**
         * @brief 해당 파티클 LOD의 활성 여부를 설정합니다.
         * @param[in] Enabled 해당 기능의 활성 여부입니다.
         */
        void SetEnabled(bool Enabled);

        /**
         * @brief LOD가 소유하는 필수 모듈 참조를 반환합니다.
         * @return 필수 모듈의 비소유 참조입니다.
         */
        CoreUObject::TObjectRef<Engine::UParticleModuleRequired> GetRequiredModule();

        /**
         * @brief LOD가 소유하는 필수 모듈 참조를 반환합니다.
         * @return 필수 모듈의 비소유 참조입니다.
         */
        CoreUObject::TObjectRef<const Engine::UParticleModuleRequired> GetRequiredModule() const;

        /**
         * @brief 필수 모듈의 소유권을 받아 LOD에 연결합니다.
         * @param[in] Module LOD에 소유권을 이전할 파티클 모듈입니다.
         * @return 필수 모듈을 교체했으면 true입니다.
         */
        bool SetRequiredModule(std::unique_ptr<Engine::UParticleModuleRequired> Module);

        /**
         * @brief 파티클 모듈의 소유권을 받아 LOD 실행 목록에 연결합니다.
         * @param[in] Module LOD에 소유권을 이전할 파티클 모듈입니다.
         * @return 유효한 모듈을 추가했으면 true입니다.
         */
        bool AddModule(std::unique_ptr<Engine::UParticleModule> Module);

        /**
         * @brief 지정한 모듈을 찾아 제거하고 실행 목록과 빌드 캐시를 갱신합니다.
         * @param[in] Module 조회하거나 제거할 모듈의 비소유 참조입니다.
         * @return 대상 모듈을 제거했으면 true입니다.
         */
        bool RemoveModule(CoreUObject::TObjectRef<const Engine::UParticleModule> Module);

        /**
         * @brief 일반 모듈을 제거하고 실행 목록과 이미터 배치 캐시를 갱신합니다.
         */
        void ClearModules();

        /**
         * @brief 인덱스로 일반 모듈의 형식 지정 참조를 반환합니다.
         * @param[in] Index 조회하거나 변경할 0부터 시작하는 인덱스입니다.
         * @return 모듈의 비소유 참조이며 범위를 벗어나면 빈 참조입니다.
         */
        CoreUObject::TObjectRef<Engine::UParticleModule> GetModule(Index_t Index);

        /**
         * @brief 인덱스로 일반 모듈의 형식 지정 참조를 반환합니다.
         * @param[in] Index 조회하거나 변경할 0부터 시작하는 인덱스입니다.
         * @return 모듈의 비소유 참조이며 범위를 벗어나면 빈 참조입니다.
         */
        CoreUObject::TObjectRef<const Engine::UParticleModule> GetModule(Index_t Index) const;

        /**
         * @brief LOD가 소유하는 일반 모듈 수를 반환합니다.
         * @return 필수 모듈을 제외한 모듈 수입니다.
         */
        std::size_t GetModuleCount() const;

        /**
         * @brief LOD의 일반 모듈을 비소유 참조 목록으로 반환합니다.
         * @return 등록 순서의 모듈 참조 목록입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> GetModules() const;

        /**
         * @brief 활성 모듈의 플래그로 생성·갱신·최종 갱신 목록을 다시 구성합니다.
         */
        virtual void UpdateModuleLists();

        /**
         * @brief 파티클 생성 단계에서 실행할 모듈 목록을 반환합니다.
         * @return 생성 모듈 참조 목록입니다.
         */
        const Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>>& GetSpawnModules() const;

        /**
         * @brief 파티클 갱신 단계에서 실행할 모듈 목록을 반환합니다.
         * @return 갱신 모듈 참조 목록입니다.
         */
        const Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>>& GetUpdateModules() const;

        /**
         * @brief 최종 갱신 단계에서 실행할 모듈 목록을 반환합니다.
         * @return 최종 갱신 모듈 참조 목록입니다.
         */
        const Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>>& GetFinalUpdateModules() const;

        void Serialize(Core::FArchive& Archive) override;

    private:
        /**
         * @brief 직렬화된 클래스 정보로 파티클 모듈을 생성합니다.
         * @param[in,out] Archive 읽기·쓰기 모드를 제공하는 아카이브입니다.
         * @return 생성한 모듈의 소유권이며 생성할 수 없으면 빈 포인터입니다.
         */
        std::unique_ptr<Engine::UParticleModule> CreateModuleFromArchive(Core::FArchive& Archive);

        /**
         * @brief 일반 모듈 목록의 클래스와 데이터를 아카이브로 읽거나 씁니다.
         * @param[in,out] Archive 읽기·쓰기 모드를 제공하는 아카이브입니다.
         * @param[in,out] Modules 직렬화할 소유 모듈 목록입니다.
         */
        void SerializeModules(Core::FArchive& Archive, Core::TArray<std::unique_ptr<Engine::UParticleModule>>& Modules);

        /**
         * @brief 소유한 필수·일반 모듈의 Outer를 현재 LOD로 연결합니다.
         */
        void AdoptModules();

        /**
         * @brief 상위 이미터에 모듈 구성 변경을 알려 배치 캐시를 무효화합니다.
         */
        void InvalidateEmitterBuildData();

    private:
        /**
         * @brief 해당 파티클 LOD의 활성 여부입니다.
         */
        UPROPERTY(Edit, Getter = GetEnabled, Setter = SetEnabled)
        bool mEnabled{true};

        /**
         * @brief LOD의 필수 파티클 모듈 소유 객체입니다.
         */
        std::unique_ptr<Engine::UParticleModuleRequired> mRequiredModule{};

        /**
         * @brief LOD가 소유하는 일반 파티클 모듈 목록입니다.
         */
        Core::TArray<std::unique_ptr<Engine::UParticleModule>> mModules{};

        /**
         * @brief 파티클 생성 단계에서 실행할 모듈의 비소유 참조 목록입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> mSpawnModules{};

        /**
         * @brief 파티클 갱신 단계에서 실행할 모듈의 비소유 참조 목록입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> mUpdateModules{};

        /**
         * @brief 최종 갱신 단계에서 실행할 모듈의 비소유 참조 목록입니다.
         */
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> mFinalUpdateModules{};
    };

}
