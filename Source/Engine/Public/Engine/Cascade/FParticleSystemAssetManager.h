#pragma once

#include "Core/Types.h"

#include <filesystem>
#include <memory>

namespace Asset {
    class FAssetManager;
}

namespace Engine {

    class UParticleSystem;

    class FParticleSystemAssetManager {
    public:
        explicit FParticleSystemAssetManager(Asset::FAssetManager& Assets);
        ~FParticleSystemAssetManager();

        FParticleSystemAssetManager(const Engine::FParticleSystemAssetManager&) = delete;
        Engine::FParticleSystemAssetManager& operator=(const Engine::FParticleSystemAssetManager&) = delete;

    public:
        Engine::UParticleSystem* LoadParticleSystem(const std::filesystem::path& Path, Core::FString& Error);
        Engine::UParticleSystem* CreateParticleSystem(const std::filesystem::path& Directory, const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error);
        bool SaveParticleSystem(const std::filesystem::path& Path, Engine::UParticleSystem& System, Core::FString& Error);
        void RetireAssets(const std::filesystem::path& Path);

    private:
        Core::FString MakeCacheKey(const Core::FString& Relative) const;
        bool ResolveMaterials(Engine::UParticleSystem& System, Core::FString& Error);

    private:
        Asset::FAssetManager& mAssets;
        Core::TMap<Core::FString, std::unique_ptr<Engine::UParticleSystem>> mParticleSystems{};
        Core::TArray<std::unique_ptr<Engine::UParticleSystem>> mRetired{};
    };

}
