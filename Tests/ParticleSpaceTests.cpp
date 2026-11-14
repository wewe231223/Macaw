#include "Core/Math/FTransform.h"
#include "Core/Math/FVector4.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/Cascade/FBaseParticle.h"
#include "Engine/Cascade/FParticleEmitterInstance.h"
#include "Engine/Cascade/IParticleEmitterInstanceOwner.h"
#include "Engine/Cascade/UParticleEmitter.h"
#include "Engine/Cascade/UParticleLODLevel.h"
#include "Engine/Cascade/UParticleModule.h"
#include "Engine/Cascade/UParticleModuleRequired.h"
#include "Serialization/MemoryArchive.h"

#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace Core;
using namespace CoreUObject;
using namespace Engine;

namespace {

    void Require(bool Condition, const FString& Message) {
        if (!Condition) {
            throw std::runtime_error{Message};
        }

        std::cout << "PASS " << Message << '\n';
    }

    void RequireVector(const FVector& Actual, const FVector& Expected, const FString& Message) {
        Require(Actual.Equals(Expected, 0.001f), Message);
    }

    class FTestOwner : public IParticleEmitterInstanceOwner {
    public:
        const FTransform& GetTransform() const override;
        FString GetInstanceName() const override;
        void SetTransform(const FTransform& Transform);

    private:
        FTransform mTransform{};
    };

    const FTransform& FTestOwner::GetTransform() const {
        return mTransform;
    }

    FString FTestOwner::GetInstanceName() const {
        return "ParticleSpaceTests";
    }

    void FTestOwner::SetTransform(const FTransform& Transform) {
        mTransform = Transform;
    }

    struct FSpawnSnapshot {
    public:
        FSpawnSnapshot() = default;
        FSpawnSnapshot(const FVector& Location, const FVector& Velocity, float SpawnTime);

    public:
        const FVector& GetLocation() const;
        const FVector& GetVelocity() const;
        float GetSpawnTime() const;

    private:
        FVector mLocation{};
        FVector mVelocity{};
        float mSpawnTime{};
    };

    FSpawnSnapshot::FSpawnSnapshot(const FVector& Location, const FVector& Velocity, float SpawnTime)
		: mLocation{Location},
		  mVelocity{Velocity},
		  mSpawnTime{SpawnTime} {
    }

    const FVector& FSpawnSnapshot::GetLocation() const {
        return mLocation;
    }

    const FVector& FSpawnSnapshot::GetVelocity() const {
        return mVelocity;
    }

    float FSpawnSnapshot::GetSpawnTime() const {
        return mSpawnTime;
    }

    class UProbeModule : public UParticleModule {
    public:
        UProbeModule();

    public:
        Uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
        void Spawn(const FSpawnContext& Context) override;
    };

    UProbeModule::UProbeModule() {
        mSpawnModule = true;
    }

    Uint32 UProbeModule::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
        return sizeof(FSpawnSnapshot);
    }

    void UProbeModule::Spawn(const FSpawnContext& Context) {
        const FBaseParticle* Particle{Context.GetParticleBase()};
        const FSpawnSnapshot Snapshot{Particle->GetLocation(), Particle->GetVelocity(), Context.GetSpawnTime()};

        std::memcpy(reinterpret_cast<Uint8*>(Context.GetParticleBase()) + Context.GetOffset(), &Snapshot, sizeof(Snapshot));
    }

    void TestRequiredSettings() {
        auto Required{NewObject<UParticleModuleRequired>()};
        Require(Required->GetEmitterOrigin() == FVector{} && Required->GetEmitterRotation().Equals({}), "Required offsets default to zero");
        Require(Required->SetEmitterOrigin({2.0f, 3.0f, 4.0f}) && Required->SetEmitterRotation({10.0f, 90.0f, 20.0f}), "Set Required emitter origin and rotation");
        const float Invalid{std::numeric_limits<float>::infinity()};
        Require(!Required->SetEmitterOrigin({Invalid, 0.0f, 0.0f}) && !Required->SetEmitterRotation({0.0f, Invalid, 0.0f}) && Required->GetEmitterOrigin() == FVector{2.0f, 3.0f, 4.0f} && Required->GetEmitterRotation().Equals({10.0f, 90.0f, 20.0f}), "Reject invalid offsets without changing Required settings");
        Required->SetUseLocalSpace(true);
        TArray<Uint8> Bytes{};
        Serialization::FMemoryArchive Writer{Bytes};
        Required->Serialize(Writer);
        auto Copy{NewObject<UParticleModuleRequired>()};
        Serialization::FMemoryArchive Reader{static_cast<const TArray<Uint8>&>(Bytes)};
        Copy->Serialize(Reader);
        Require(!Writer.HasError() && !Reader.HasError() && Reader.GetRemainingBytes() == 0 && Copy->GetEmitterOrigin() == Required->GetEmitterOrigin() && Copy->GetEmitterRotation().Equals(Required->GetEmitterRotation()) && Copy->GetUseLocalSpace(), "Required offsets and local-space setting survive serialization");
    }

    void TestSpawnAndMovement(bool UseLocalSpace) {
        FTestOwner Owner{};
        FTransform Transform{{10.0f, 20.0f, 30.0f}, FQuat{FVector{0.0f, 0.0f, 1.0f}, 1.57079632679f}, {2.0f, -3.0f, 4.0f}};
        Owner.SetTransform(Transform);
        auto Emitter{NewObject<UParticleEmitter>()};
        const auto Required{Emitter->GetLODLevel(0)->GetRequiredModule()};
        Required->SetUseLocalSpace(UseLocalSpace);
        Required->SetEmitterOrigin({2.0f, 0.0f, 0.0f});
        Required->SetEmitterRotation({0.0f, 90.0f, 0.0f});
        auto Module{std::make_unique<UProbeModule>()};
        const TObjectRef<UParticleModule> ModuleReference{Module.get()};
        Require(Emitter->GetLODLevel(0)->AddModule(std::move(Module)), "Attach spawn probe");
        auto Instance{Emitter->CreateInstance(Owner)};
        Require(Instance->GetUseLocalSpace() == UseLocalSpace, "Read simulation space from current Required module");
        const FVector ExpectedLocation{UseLocalSpace ? FVector{2.0f, 1.0f, 0.0f} : FVector{9.0f, 22.0f, 30.0f}};
        const FVector ExpectedVelocity{UseLocalSpace ? FVector{0.0f, 1.0f, 0.0f} : FVector{-1.0f, 0.0f, 0.0f}};
        const FVector InitialLocation{Instance->GetEmitterToSimulation().TransformPosition({1.0f, 0.0f, 0.0f})};
        const FVector InitialVelocity{Instance->GetEmitterToSimulation().TransformVector({1.0f, 0.0f, 0.0f})};
        RequireVector(InitialLocation, ExpectedLocation, "Compose emitter offsets and component transform in the correct order");
        RequireVector(InitialVelocity, ExpectedVelocity, "Transform direction without translation or owner scale");
        RequireVector(Instance->GetSimulationToWorld().TransformPosition(InitialLocation), {9.0f, 22.0f, 30.0f}, "Both simulation modes produce the same initial world position");
        Require(Instance->SpawnParticle(0.25f, InitialLocation, InitialVelocity) == 0, "Spawn with prepared simulation-space data");
        RequireVector(Instance->GetParticle(0)->GetLocation(), ExpectedLocation, "Spawn does not transform initial position again");
        RequireVector(Instance->GetParticle(0)->GetVelocity(), ExpectedVelocity, "Spawn does not transform initial velocity again");
        FSpawnSnapshot Snapshot{};
        std::memcpy(&Snapshot, Instance->GetParticleModuleData(0, ModuleReference), sizeof(Snapshot));
        Require(Snapshot.GetLocation().Equals(ExpectedLocation, 0.001f) && Snapshot.GetVelocity().Equals(ExpectedVelocity, 0.001f) && Snapshot.GetSpawnTime() == 0.25f, "Spawn modules receive simulation-space data and time");
        const FMatrix OriginalEmitterToSimulation{Instance->GetEmitterToSimulation()};
        const FMatrix OriginalSimulationToWorld{Instance->GetSimulationToWorld()};
        Transform.mScale = {};
        Owner.SetTransform(Transform);
        Require(Instance->UpdateTransforms() && Instance->GetEmitterToSimulation().Equals(OriginalEmitterToSimulation) && Instance->GetSimulationToWorld().Equals(OriginalSimulationToWorld), "Owner scale is excluded even when zero");
        Owner.SetTransform({{100.0f, 200.0f, 300.0f}, {}, {7.0f, 8.0f, 9.0f}});
        Require(Instance->SpawnParticle(0.5f, {77.0f, 88.0f, 99.0f}, {7.0f, 8.0f, 9.0f}) == 1, "Spawn accepts arbitrary simulation-space data");
        Require(Instance->GetEmitterToSimulation().Equals(OriginalEmitterToSimulation) && Instance->GetSimulationToWorld().Equals(OriginalSimulationToWorld), "Spawn leaves transform refresh to the frame update");
        RequireVector(Instance->GetParticle(1)->GetLocation(), {77.0f, 88.0f, 99.0f}, "Simulation-space position is stored verbatim");
        RequireVector(Instance->GetParticle(1)->GetVelocity(), {7.0f, 8.0f, 9.0f}, "Simulation-space velocity is stored verbatim");
        Require(Instance->UpdateTransforms(), "Refresh moving emitter without spawning");
        RequireVector(Instance->GetParticle(0)->GetLocation(), ExpectedLocation, "Moving emitter leaves stored particle data unchanged");
        RequireVector(Instance->GetSimulationToWorld().TransformPosition(Instance->GetParticle(0)->GetLocation()), UseLocalSpace ? FVector{102.0f, 201.0f, 300.0f} : FVector{9.0f, 22.0f, 30.0f}, "Local particles follow emitter while world particles stay fixed");
        const FVector Origin{Instance->GetEmitterToSimulation().TransformPosition({})};
        Require(Instance->SpawnParticle(0.0f, Origin, {}) == 2, "Caller supplies current emitter origin in simulation space");
        RequireVector(Instance->GetSimulationToWorld().TransformPosition(Instance->GetParticle(2)->GetLocation()), {102.0f, 200.0f, 300.0f}, "New particle uses updated emitter origin");
    }

    void TestSpaceChanges() {
        FTestOwner Owner{};
        Owner.SetTransform({{10.0f, 20.0f, 30.0f}, FQuat{FVector{0.0f, 0.0f, 1.0f}, 1.57079632679f}, {2.0f, 3.0f, 4.0f}});
        auto Emitter{NewObject<UParticleEmitter>()};
        auto Module{std::make_unique<UProbeModule>()};
        const TObjectRef<UParticleModule> ModuleReference{Module.get()};
        Emitter->GetLODLevel(0)->AddModule(std::move(Module));
        auto LocalLOD{NewObject<UParticleLODLevel>()};
        LocalLOD->GetRequiredModule()->SetUseLocalSpace(true);
        Require(Emitter->AddLODLevel(std::move(LocalLOD)), "Add LOD with local simulation");
        auto Instance{Emitter->CreateInstance(Owner)};
        Require(Instance->SpawnParticle(0.5f, {12.0f, 26.0f, 42.0f}, {6.0f, 6.0f, 4.0f}) == 0, "Spawn before space changes");
        FBaseParticle* Particle{Instance->GetParticle(0)};
        Particle->GetColor() = {0.2f, 0.3f, 0.4f, 0.5f};
        Particle->GetRelativeTime() = 0.75f;
        FSpawnSnapshot Before{};
        std::memcpy(&Before, Instance->GetParticleModuleData(0, ModuleReference), sizeof(Before));
        Require(Instance->SetCurrentLODIndex(1) && Instance->GetUseLocalSpace(), "LOD selection reads new Required setting immediately");
        Require(Instance->GetSimulationToWorld().IsIdentity(), "LOD selection leaves matrix refresh to frame update");
        Require(Instance->UpdateTransforms(), "Refresh matrices after LOD selection");
        RequireVector(Particle->GetLocation(), {12.0f, 26.0f, 42.0f}, "LOD change does not rebase existing particle position");
        RequireVector(Particle->GetVelocity(), {6.0f, 6.0f, 4.0f}, "LOD change does not rebase existing particle velocity");
        RequireVector(Instance->GetSimulationToWorld().TransformPosition(Particle->GetLocation()), {-16.0f, 32.0f, 72.0f}, "World position follows selected space without transition compensation");
        Require(Instance->SetCurrentLODIndex(0) && !Instance->GetUseLocalSpace() && Instance->UpdateTransforms(), "Return to world simulation");
        RequireVector(Particle->GetLocation(), {12.0f, 26.0f, 42.0f}, "Returning to world space leaves position untouched");
        const auto Required{Emitter->GetLODLevel(0)->GetRequiredModule()};
        Required->SetUseLocalSpace(true);
        Require(Instance->GetUseLocalSpace() && Instance->UpdateTransforms(), "Read edited local-space setting without caching the flag");
        Required->SetEmitterOrigin({3.0f, 4.0f, 5.0f});
        Required->SetEmitterRotation({90.0f, 0.0f, 0.0f});
        Require(Instance->UpdateTransforms(), "Refresh edited emitter offsets");
        RequireVector(Particle->GetLocation(), {12.0f, 26.0f, 42.0f}, "Settings changes never rewrite stored particle position");
        RequireVector(Particle->GetVelocity(), {6.0f, 6.0f, 4.0f}, "Settings changes never rewrite stored particle velocity");
        FSpawnSnapshot After{};
        std::memcpy(&After, Instance->GetParticleModuleData(0, ModuleReference), sizeof(After));
        Require(Particle->GetColor() == FVector4{0.2f, 0.3f, 0.4f, 0.5f} && Particle->GetRelativeTime() == 0.75f && std::memcmp(&Before, &After, sizeof(Before)) == 0, "Matrix refresh preserves other particle properties and module payload");
        Require(!Instance->SetCurrentLODIndex(20) && Instance->GetCurrentLODIndex() == 0, "Reject invalid LOD index");
        Instance->Init();
        Require(Instance->GetParticleCount() == 0 && Instance->GetUseLocalSpace() && !Instance->GetSimulationToWorld().IsIdentity(), "Reinitialization clears particles and refreshes selected matrices");
        Instance->InitParameters(nullptr);
        Require(!Instance->GetUseLocalSpace() && Instance->GetEmitterToSimulation().IsIdentity() && Instance->GetSimulationToWorld().IsIdentity(), "Reset clears both cached matrices");
    }

    void TestInvalidTransforms() {
        FTestOwner Owner{};
        auto Emitter{NewObject<UParticleEmitter>()};
        auto Instance{Emitter->CreateInstance(Owner)};
        Require(Instance->SpawnParticle(0.0f, {1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}) == 0, "Spawn before invalid transform update");
        const Index_t WideIndex{static_cast<Index_t>(std::numeric_limits<Uint32>::max()) + 1};
        Require(Instance->GetParticle(WideIndex) == nullptr && static_cast<const FParticleEmitterInstance&>(*Instance).GetParticle(WideIndex) == nullptr, "Reject particle indices beyond the 32-bit range without truncation");
        const FMatrix OriginalEmitterToSimulation{Instance->GetEmitterToSimulation()};
        const FMatrix OriginalSimulationToWorld{Instance->GetSimulationToWorld()};

        for (const FTransform& Invalid : {FTransform{{}, FQuat{0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, FTransform{{std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}}}) {
            Owner.SetTransform(Invalid);
            Require(!Instance->UpdateTransforms() && Instance->GetEmitterToSimulation().Equals(OriginalEmitterToSimulation) && Instance->GetSimulationToWorld().Equals(OriginalSimulationToWorld), "Reject invalid rotation or translation without corrupting cached matrices");
        }

        Owner.SetTransform({});
        Require(Instance->UpdateTransforms(), "Recover after invalid owner transform");
        const float Invalid{std::numeric_limits<float>::infinity()};
        Require(Instance->SpawnParticle(0.0f, {Invalid, 0.0f, 0.0f}, {}) == std::numeric_limits<Index_t>::max() && Instance->SpawnParticle(0.0f, {}, {0.0f, Invalid, 0.0f}) == std::numeric_limits<Index_t>::max() && Instance->GetParticleCount() == 1, "Reject non-finite spawn data without allocating particles");
        const FVector Extreme{std::numeric_limits<float>::max(), 0.0f, 0.0f};
        Require(Instance->SpawnParticle(0.0f, Extreme, Extreme) == 1, "Spawn finite data near floating-point limits");
        Emitter->GetLODLevel(0)->GetRequiredModule()->SetUseLocalSpace(true);
        Owner.SetTransform({{}, {}, {0.5f, 1.0f, 1.0f}});
        Require(Instance->UpdateTransforms() && Instance->GetParticle(1)->GetLocation() == Extreme && Instance->GetParticle(1)->GetVelocity() == Extreme, "Space changes do not scan or convert existing particles");
    }

}

int main() {
    try {
        TestRequiredSettings();
        TestSpawnAndMovement(false);
        TestSpawnAndMovement(true);
        TestSpaceChanges();
        TestInvalidTransforms();
    } catch (const std::exception& Error) {
        std::cerr << "FAIL " << Error.what() << '\n';

        return 1;
    }

    return 0;
}
