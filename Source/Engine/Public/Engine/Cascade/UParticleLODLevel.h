#pragma once

#include "Core/Types.h"
#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/UObject.h"

#include <cstddef>
#include <memory>

#include "UParticleLODLevel.generated.h"

namespace Core {
    class FArchive;
}

namespace Engine {

    class UParticleModule;
    class UParticleModuleRequired;

    UCLASS()
    class UParticleLODLevel : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        UParticleLODLevel();
        ~UParticleLODLevel() override;

    public:
        bool GetEnabled() const;
        void SetEnabled(bool Enabled);

        CoreUObject::TObjectRef<Engine::UParticleModuleRequired> GetRequiredModule();
        CoreUObject::TObjectRef<const Engine::UParticleModuleRequired> GetRequiredModule() const;
        bool SetRequiredModule(std::unique_ptr<Engine::UParticleModuleRequired> Module);

        bool AddModule(std::unique_ptr<Engine::UParticleModule> Module);
        bool RemoveModule(CoreUObject::TObjectRef<const Engine::UParticleModule> Module);
        void ClearModules();

        CoreUObject::TObjectRef<Engine::UParticleModule> GetModule(Core::Index_t Index);
        CoreUObject::TObjectRef<const Engine::UParticleModule> GetModule(Core::Index_t Index) const;
        std::size_t GetModuleCount() const;
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> GetModules() const;

        virtual void UpdateModuleLists();
        const Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>>& GetSpawnModules() const;
        const Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>>& GetUpdateModules() const;
        const Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>>& GetFinalUpdateModules() const;

        void Serialize(Core::FArchive& Archive) override;

    private:
        std::unique_ptr<Engine::UParticleModule> CreateModuleFromArchive(Core::FArchive& Archive);
        void SerializeModules(Core::FArchive& Archive, Core::TArray<std::unique_ptr<Engine::UParticleModule>>& Modules);
        void AdoptModules();
        void InvalidateEmitterBuildData();

    private:
        UPROPERTY(Edit, Getter = GetEnabled, Setter = SetEnabled)
        bool mEnabled{true};

        std::unique_ptr<Engine::UParticleModuleRequired> mRequiredModule{};
        Core::TArray<std::unique_ptr<Engine::UParticleModule>> mModules{};

        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> mSpawnModules{};
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> mUpdateModules{};
        Core::TArray<CoreUObject::TObjectRef<Engine::UParticleModule>> mFinalUpdateModules{};
    };

}
