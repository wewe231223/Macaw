#pragma once

#include "Engine/Cascade/UParticleModule.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FVector.h"
#include "CoreUObject/TObjectRef.h"

#include "UParticleModuleRequired.generated.h"

namespace Asset {
    class FAssetManager;
    class UMaterial;
}

namespace Engine {

    UCLASS()
    class UParticleModuleRequired : public Engine::UParticleModule {
        GENERATED_BODY()

    public:
        UParticleModuleRequired();
        ~UParticleModuleRequired() override;

    public:
        Asset::UMaterial* GetMaterial() const;
        bool SetMaterial(Asset::UMaterial* Material, const Asset::FAssetManager& Assets);
        bool ResolveMaterial(Asset::FAssetManager& Assets, Core::FString& Error);

        const Core::FString& GetMaterialPath() const;
        bool SetMaterialPath(const Core::FString& Path);

        const Core::FVector& GetEmitterOrigin() const;
        bool SetEmitterOrigin(const Core::FVector& Origin);
        const Core::FRotator& GetEmitterRotation() const;
        bool SetEmitterRotation(const Core::FRotator& Rotation);

        bool GetUseLocalSpace() const;
        void SetUseLocalSpace(bool UseLocalSpace);

        float GetEmitterDuration() const;
        bool SetEmitterDuration(float Duration);

        Core::Uint32 GetEmitterLoops() const;
        void SetEmitterLoops(Core::Uint32 Loops);

    private:
        CoreUObject::TObjectRef<Asset::UMaterial> mMaterial{};

        UPROPERTY(Edit, Serialize, Name = "Material", Getter = GetMaterialPath, Setter = SetMaterialPath)
        Core::FString mMaterialPath{};

        UPROPERTY(Edit, Serialize, Getter = GetEmitterOrigin, Setter = SetEmitterOrigin)
        Core::FVector mEmitterOrigin{};

        UPROPERTY(Edit, Serialize, Getter = GetEmitterRotation, Setter = SetEmitterRotation)
        Core::FRotator mEmitterRotation{};

        UPROPERTY(Edit, Serialize, Getter = GetUseLocalSpace, Setter = SetUseLocalSpace)
        bool mUseLocalSpace{};

        UPROPERTY(Edit, Serialize, Getter = GetEmitterDuration, Setter = SetEmitterDuration)
        float mEmitterDuration{1.0f};

        UPROPERTY(Edit, Serialize, Getter = GetEmitterLoops, Setter = SetEmitterLoops)
        Core::Uint32 mEmitterLoops{};
    };

}
