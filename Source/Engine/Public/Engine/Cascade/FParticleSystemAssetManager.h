/**
 * @file FParticleSystemAssetManager.h
 * @brief 파티클 시스템 에셋의 로드 캐시와 참조 머티리얼 해석을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Asset/FAssetManager.h"
#include "Engine/Cascade/UParticleSystem.h"

#include <filesystem>
#include <memory>

namespace Engine {

    /**
     * @brief 파티클 시스템 에셋의 로드 캐시와 참조 머티리얼 해석을 관리합니다.
     */
    class FParticleSystemAssetManager {
    public:
        /**
         * @brief FParticleSystemAssetManager의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Assets 참조 에셋의 경로 조회·로드를 제공하는 관리자입니다.
         */
        explicit FParticleSystemAssetManager(Asset::FAssetManager& Assets);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FParticleSystemAssetManager();

        /**
         * @brief 중복된 수명·연결 상태를 방지하기 위해 복사 생성을 금지합니다.
         */
        FParticleSystemAssetManager(const Engine::FParticleSystemAssetManager&) = delete;

        /**
         * @brief 중복된 수명·연결 상태를 방지하기 위해 대입을 금지합니다.
         */
        Engine::FParticleSystemAssetManager& operator=(const Engine::FParticleSystemAssetManager&) = delete;

    public:
        Engine::UParticleSystem* LoadParticleSystem(const std::filesystem::path& Path, Core::FString& Error);
        Engine::UParticleSystem* CreateParticleSystem(const std::filesystem::path& Directory, const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error);
        bool SaveParticleSystem(const std::filesystem::path& Path, Engine::UParticleSystem& System, Core::FString& Error);

        /**
         * @brief 지정 경로의 파티클 시스템 캐시를 폐기 목록으로 옮깁니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         */
        void RetireAssets(const std::filesystem::path& Path);

    private:
        /**
         * @brief 프로젝트 상대 경로를 정규화된 파티클 시스템 캐시 키로 바꿉니다.
         * @param[in] Relative 캐시 키로 변환할 프로젝트 상대 경로입니다.
         * @return 캐시 조회에 사용할 키입니다.
         */
        Core::FString MakeCacheKey(const Core::FString& Relative) const;

        /**
         * @brief 파티클 시스템의 모든 LOD 필수 모듈이 참조하는 머티리얼을 로드합니다.
         * @param[in] System 머티리얼을 해석할 파티클 시스템입니다.
         * @param[out] Error 실패 원인을 전달하는 오류 문자열입니다.
         * @return 모든 머티리얼을 해석했으면 true입니다.
         */
        bool ResolveMaterials(Engine::UParticleSystem& System, Core::FString& Error);

    private:
        /**
         * @brief 참조 에셋을 로드하는 공유 에셋 관리자입니다.
         */
        Asset::FAssetManager& mAssets;

        /**
         * @brief 정규화된 에셋 경로별 파티클 시스템 소유 캐시입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Engine::UParticleSystem>> mParticleSystems{};

        /**
         * @brief 캐시에서 폐기했지만 기존 참조를 위해 수명을 유지하는 에셋 목록입니다.
         */
        Core::TArray<std::unique_ptr<Engine::UParticleSystem>> mRetired{};
    };

}
