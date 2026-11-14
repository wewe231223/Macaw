#pragma once

#include "Core/Types.h"
#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/UObject.h"
#include "Engine/Cascade/FParticleEmitterInstance.h"

#include <cstddef>
#include <memory>

#include "UParticleEmitter.generated.h"

namespace Core {
    class FArchive;
}

namespace Engine {

    class UParticleLODLevel;
    class UParticleModule;
    class IParticleEmitterInstanceOwner;

    UCLASS()
    class UParticleEmitter : public CoreUObject::UObject {
        GENERATED_BODY()

    private:
        friend class Engine::UParticleLODLevel;

    public:
        UParticleEmitter();
        ~UParticleEmitter() override;

    public:
        bool AddLODLevel(std::unique_ptr<Engine::UParticleLODLevel> LODLevel);
        bool RemoveLODLevel(CoreUObject::TObjectRef<const Engine::UParticleLODLevel> LODLevel);
        void ClearLODLevels();

        CoreUObject::TObjectRef<Engine::UParticleLODLevel> GetLODLevel(Core::Index_t Index);
        CoreUObject::TObjectRef<const Engine::UParticleLODLevel> GetLODLevel(Core::Index_t Index) const;
        std::size_t GetLODLevelCount() const;
        const Core::TArray<std::unique_ptr<Engine::UParticleLODLevel>>& GetLODLevels() const;
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> GetAllModules() const;

        void Build();
        void CacheEmitterModuleInfo();
        virtual std::unique_ptr<Engine::FParticleEmitterInstance> CreateInstance(Engine::IParticleEmitterInstanceOwner& Owner);

        Core::Uint32 GetParticleSize() const;
        Core::Uint32 GetInstancePayloadSize() const;
        const Core::TMap<CoreUObject::TObjectRef<const Engine::UParticleModule>, Core::Index_t, CoreUObject::TObjectRefHash<const Engine::UParticleModule>>& GetModuleDataOffsets() const;
        const Core::TMap<CoreUObject::TObjectRef<const Engine::UParticleModule>, Core::Index_t, CoreUObject::TObjectRefHash<const Engine::UParticleModule>>& GetModuleInstanceDataOffsets() const;

        void Serialize(Core::FArchive& Archive) override;

    private:
        void InvalidateBuildData();

    private:
        Core::TArray<std::unique_ptr<Engine::UParticleLODLevel>> mLODLevels{};
        Core::TMap<CoreUObject::TObjectRef<const Engine::UParticleModule>, Core::Index_t, CoreUObject::TObjectRefHash<const Engine::UParticleModule>> mModuleDataOffsets{};
        Core::TMap<CoreUObject::TObjectRef<const Engine::UParticleModule>, Core::Index_t, CoreUObject::TObjectRefHash<const Engine::UParticleModule>> mModuleInstanceDataOffsets{};
        Core::Uint32 mParticleSize{};
        Core::Uint32 mInstancePayloadSize{};
    };

}
