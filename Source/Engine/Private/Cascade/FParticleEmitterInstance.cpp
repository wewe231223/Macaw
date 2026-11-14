/**
 * @file FParticleEmitterInstance.cpp
 * @brief 인스턴스의 원본 참조와 파티클 버퍼 및 모듈 실행 페이로드의 수명을 구현합니다.
 * @ingroup Cascade
 */

#include "Core/Types.h"
#include "Engine/Cascade/FParticleEmitterInstance.h"
#include "Engine/Cascade/FBaseParticle.h"
#include "Engine/Cascade/UParticleEmitter.h"
#include "Engine/Cascade/UParticleLODLevel.h"
#include "Engine/Cascade/UParticleModule.h"
#include "Engine/Cascade/UParticleModuleRequired.h"
#include "Engine/Cascade/IParticleEmitterInstanceOwner.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"

#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

using namespace Engine;
using namespace Core;

/**
 * @brief 정렬 할당과 짝을 이루는 연산자로 페이로드 저장 공간을 해제합니다.
 */
void FParticleEmitterInstance::FPayloadDeleter::operator()(Uint8* Data) const noexcept {
    ::operator delete(Data, std::align_val_t{16});
}

/**
 * @brief 소유자 인터페이스의 비소유 참조를 보관하는 인스턴스를 구성합니다.
 */
FParticleEmitterInstance::FParticleEmitterInstance(IParticleEmitterInstanceOwner& Owner)
	: mOwner{Owner} {
}

/**
 * @brief 남은 파티클을 파괴하고 파티클 버퍼와 모듈 페이로드를 해제합니다.
 */
FParticleEmitterInstance::~FParticleEmitterInstance() {
    ClearParticles();
}

/**
 * @brief 원본 Emitter 참조를 교체하고 기존 페이로드와 배치 스냅샷을 비웁니다.
 */
void FParticleEmitterInstance::InitParameters(UParticleEmitter* Template) {
    ResetRuntimeData();
    mTemplate = Template;
}

/**
 * @brief 기존 파티클을 비우고 원본 정의의 배치와 모듈별 인스턴스 페이로드를 준비합니다.
 */
void FParticleEmitterInstance::Init() {
    const Index_t LODIndex{mCurrentLODIndex >= 0 ? mCurrentLODIndex : 0};

    ResetRuntimeData();

    auto* Template{GetTemplate()};

    if (Template == nullptr) {
        return;
    }

    Template->Build();
    mModules = Template->GetAllModules();
    mModuleDataOffsets = Template->GetModuleDataOffsets();
    mModuleInstanceDataOffsets = Template->GetModuleInstanceDataOffsets();
    mParticleStride = Template->GetParticleSize();
    mInstancePayloadSize = Template->GetInstancePayloadSize();

    for (const auto& LODLevel : Template->GetLODLevels()) {
        mLODLevels.push_back(LODLevel.get());
        mLODModules.push_back(LODLevel->GetModules());
    }

    if (!SetCurrentLODIndex(LODIndex)) {
        SetCurrentLODIndex(0);
    }

    UpdateTransforms();

    if (mInstancePayloadSize != 0) {
        mModuleInstancePayload.reset(static_cast<Uint8*>(::operator new(mInstancePayloadSize, std::align_val_t{16})));
        std::memset(mModuleInstancePayload.get(), 0, mInstancePayloadSize);
    }

    for (const auto& Reference : mModules) {
        UParticleModule* Module{Reference.Get()};

        if (auto* Data{GetModuleInstanceData(Reference)}; Module != nullptr && Data != nullptr) {
            Module->PrepPerInstanceBlock(this, Data);
        }
    }
}

/**
 * @brief 공간 변환과 인스턴스 이름을 제공하는 소유자를 반환합니다.
 */
IParticleEmitterInstanceOwner& FParticleEmitterInstance::GetOwner() const {
    return mOwner;
}

/**
 * @brief 세대 핸들로 원본 Emitter 참조를 해석합니다.
 */
UParticleEmitter* FParticleEmitterInstance::GetTemplate() const {
    return mTemplate.Get();
}

UParticleLODLevel* FParticleEmitterInstance::GetCurrentLODLevel() const {
    return mCurrentLODIndex >= 0 && static_cast<std::size_t>(mCurrentLODIndex) < mLODLevels.size() ? mLODLevels[mCurrentLODIndex].Get() : nullptr;
}

Index_t FParticleEmitterInstance::GetCurrentLODIndex() const {
    return mCurrentLODIndex;
}

bool FParticleEmitterInstance::SetCurrentLODIndex(Index_t Index) {
    if (Index < 0 || static_cast<std::size_t>(Index) >= mLODLevels.size() || !mLODLevels[Index].IsValid()) {
        return false;
    }

    mCurrentLODIndex = Index;

    return true;
}

bool FParticleEmitterInstance::UpdateTransforms() {
    const UParticleLODLevel* LODLevel{GetCurrentLODLevel()};

    if (LODLevel == nullptr) {
        return false;
    }

    const auto RequiredModule{LODLevel->GetRequiredModule()};
    const FTransform& Transform{mOwner.GetTransform()};

    if (RequiredModule == nullptr || !Transform.mLocation.IsFinite() || !Transform.mRotation.IsNormalized()) {
        return false;
    }

    const FMatrix ComponentToWorld{FMatrix::Rotation(Transform.mRotation) * FMatrix::Translation(Transform.mLocation)};
    const FMatrix EmitterToComponent{FMatrix::Rotation(RequiredModule->GetEmitterRotation()) * FMatrix::Translation(RequiredModule->GetEmitterOrigin())};
    const bool UseLocalSpace{RequiredModule->GetUseLocalSpace()};
    const FMatrix EmitterToSimulation{UseLocalSpace ? EmitterToComponent : EmitterToComponent * ComponentToWorld};
    const FMatrix SimulationToWorld{UseLocalSpace ? ComponentToWorld : FMatrix{}};

    if (!EmitterToSimulation.IsFinite() || !SimulationToWorld.IsFinite()) {
        return false;
    }

    mEmitterToSimulation = EmitterToSimulation;
    mSimulationToWorld = SimulationToWorld;

    return true;
}

bool FParticleEmitterInstance::GetUseLocalSpace() const {
    const UParticleLODLevel* LODLevel{GetCurrentLODLevel()};
    const auto RequiredModule{LODLevel != nullptr ? LODLevel->GetRequiredModule() : nullptr};

    return RequiredModule != nullptr && RequiredModule->GetUseLocalSpace();
}

const FMatrix& FParticleEmitterInstance::GetEmitterToSimulation() const {
    return mEmitterToSimulation;
}

const FMatrix& FParticleEmitterInstance::GetSimulationToWorld() const {
    return mSimulationToWorld;
}

/**
 * @brief 기존 파티클과 모듈 추가 데이터를 보존하면서 16바이트 정렬 버퍼의 용량을 확보합니다.
 */
void FParticleEmitterInstance::ReserveParticles(Uint32 Capacity) {
    if (Capacity <= mParticleCapacity) {
        return;
    }

    if (mParticleStride == 0) {
        throw std::logic_error{"Particle emitter instance must be initialized before reserving particles"};
    }

    if (Capacity > static_cast<Uint32>(std::numeric_limits<Index_t>::max()) || Capacity > std::numeric_limits<std::size_t>::max() / mParticleStride) {
        throw std::length_error{"Particle buffer exceeds the supported capacity"};
    }

    const std::size_t BufferSize{static_cast<std::size_t>(Capacity) * mParticleStride};
    std::unique_ptr<Uint8, FPayloadDeleter> ParticleData{static_cast<Uint8*>(::operator new(BufferSize, std::align_val_t{16}))};

    for (Index_t Index{}; Index < static_cast<Index_t>(mParticleCount); ++Index) {
        const FBaseParticle* Particle{GetParticle(Index)};
        Uint8* Record{ParticleData.get() + static_cast<std::size_t>(Index) * mParticleStride};

        std::construct_at(reinterpret_cast<FBaseParticle*>(Record), *Particle);
        std::memcpy(Record + sizeof(FBaseParticle), reinterpret_cast<const Uint8*>(Particle) + sizeof(FBaseParticle), mParticleStride - sizeof(FBaseParticle));
    }

    for (Index_t Index{}; Index < static_cast<Index_t>(mParticleCount); ++Index) {
        std::destroy_at(GetParticle(Index));
    }

    mParticleData = std::move(ParticleData);
    mParticleCapacity = Capacity;
}

/**
 * @brief 기본 데이터와 0으로 채운 추가 데이터를 생성하고 Spawn 컨텍스트로 모듈을 초기화합니다.
 */
Index_t FParticleEmitterInstance::SpawnParticle(float SpawnTime, const FVector& InitialLocation, const FVector& InitialVelocity) {
    const UParticleLODLevel* LODLevel{GetCurrentLODLevel()};

    if (mParticleStride == 0 || LODLevel == nullptr || !LODLevel->GetEnabled()) {
        return -1;
    }

    if (!InitialLocation.IsFinite() || !InitialVelocity.IsFinite()) {
        return -1;
    }

    constexpr Uint32 MaximumCapacity{static_cast<Uint32>(std::numeric_limits<Index_t>::max())};

    if (mParticleCount == MaximumCapacity) {
        throw std::length_error{"Particle buffer exceeds the supported capacity"};
    }

    if (mParticleCount == mParticleCapacity) {
        const Uint32 Capacity{mParticleCapacity == 0 ? Uint32{64} : mParticleCapacity > MaximumCapacity / 2 ? MaximumCapacity : mParticleCapacity * 2};

        ReserveParticles(Capacity);
    }

    const Index_t Index{static_cast<Index_t>(mParticleCount)};
    Uint8* Record{mParticleData.get() + static_cast<std::size_t>(Index) * mParticleStride};

    std::memset(Record, 0, mParticleStride);

    FBaseParticle* Particle{std::construct_at(reinterpret_cast<FBaseParticle*>(Record))};

    Particle->GetLocation() = InitialLocation;
    Particle->GetVelocity() = InitialVelocity;
    ++mParticleCount;

    for (const auto& Reference : mLODModules[mCurrentLODIndex]) {
        UParticleModule* Module{Reference.Get()};

        if (Module != nullptr && Module->mEnabled && Module->mSpawnModule) {
            const UParticleModule::FSpawnContext Context{*this, GetModuleDataOffset(Reference), SpawnTime, Particle};

            Module->Spawn(Context);
        }
    }

    return Index;
}

/**
 * @brief 지정한 파티클을 파괴하고 마지막 파티클의 기본 데이터와 추가 데이터로 빈자리를 채웁니다.
 */
void FParticleEmitterInstance::KillParticle(Index_t Index) {
    FBaseParticle* Particle{GetParticle(Index)};

    if (Particle == nullptr) {
        return;
    }

    const Index_t LastIndex{static_cast<Index_t>(mParticleCount - 1)};
    FBaseParticle* LastParticle{GetParticle(LastIndex)};

    if (Index != LastIndex) {
        *Particle = *LastParticle;
        std::memcpy(reinterpret_cast<Uint8*>(Particle) + sizeof(FBaseParticle), reinterpret_cast<const Uint8*>(LastParticle) + sizeof(FBaseParticle), mParticleStride - sizeof(FBaseParticle));
    }

    std::destroy_at(LastParticle);
    --mParticleCount;
}

/**
 * @brief 할당한 용량과 모듈 인스턴스 페이로드를 유지하면서 모든 활성 파티클을 파괴합니다.
 */
void FParticleEmitterInstance::ClearParticles() {
    for (Index_t Index{}; Index < static_cast<Index_t>(mParticleCount); ++Index) {
        std::destroy_at(GetParticle(Index));
    }

    mParticleCount = 0;
}

/**
 * @brief 활성 레코드 시작에 생성한 기본 파티클을 반환합니다.
 */
FBaseParticle* FParticleEmitterInstance::GetParticle(Index_t Index) {
    return Index >= 0 && static_cast<Uint32>(Index) < mParticleCount ? reinterpret_cast<FBaseParticle*>(mParticleData.get() + static_cast<std::size_t>(Index) * mParticleStride) : nullptr;
}

/**
 * @brief 활성 레코드 시작에 생성한 기본 파티클을 읽기 전용으로 반환합니다.
 */
const FBaseParticle* FParticleEmitterInstance::GetParticle(Index_t Index) const {
    return Index >= 0 && static_cast<Uint32>(Index) < mParticleCount ? reinterpret_cast<const FBaseParticle*>(mParticleData.get() + static_cast<std::size_t>(Index) * mParticleStride) : nullptr;
}

/**
 * @brief 파티클 레코드 시작과 모듈 오프셋으로 추가 데이터의 주소를 반환합니다.
 */
Uint8* FParticleEmitterInstance::GetParticleModuleData(Index_t Index, CoreUObject::TObjectRef<const UParticleModule> Module) {
    FBaseParticle* Particle{GetParticle(Index)};
    const Index_t Offset{GetModuleDataOffset(Module)};

    return Particle != nullptr && Offset != 0 ? reinterpret_cast<Uint8*>(Particle) + Offset : nullptr;
}

/**
 * @brief 파티클 레코드 시작과 모듈 오프셋으로 추가 데이터의 읽기 전용 주소를 반환합니다.
 */
const Uint8* FParticleEmitterInstance::GetParticleModuleData(Index_t Index, CoreUObject::TObjectRef<const UParticleModule> Module) const {
    const FBaseParticle* Particle{GetParticle(Index)};
    const Index_t Offset{GetModuleDataOffset(Module)};

    return Particle != nullptr && Offset != 0 ? reinterpret_cast<const Uint8*>(Particle) + Offset : nullptr;
}

/**
 * @brief 연속해서 저장한 활성 파티클 수를 반환합니다.
 */
Uint32 FParticleEmitterInstance::GetParticleCount() const {
    return mParticleCount;
}

/**
 * @brief 재할당 없이 저장할 수 있는 파티클 수를 반환합니다.
 */
Uint32 FParticleEmitterInstance::GetParticleCapacity() const {
    return mParticleCapacity;
}

/**
 * @brief 기본 데이터와 모듈 추가 데이터 및 패딩을 포함한 레코드 간격을 반환합니다.
 */
Uint32 FParticleEmitterInstance::GetParticleStride() const {
    return mParticleStride;
}

/**
 * @brief 배치 스냅샷에서 모듈의 파티클 추가 데이터 오프셋을 조회합니다.
 */
Core::Index_t FParticleEmitterInstance::GetModuleDataOffset(CoreUObject::TObjectRef<const UParticleModule> Module) const {
    const auto Found{mModuleDataOffsets.find(Module)};

    return Module.IsValid() && Found != mModuleDataOffsets.end() ? Found->second : 0;
}

/**
 * @brief 이 인스턴스에서 모듈에 배정한 실행 상태 영역을 반환합니다.
 */
Uint8* FParticleEmitterInstance::GetModuleInstanceData(CoreUObject::TObjectRef<const UParticleModule> Module) {
    const auto Found{mModuleInstanceDataOffsets.find(Module)};

    return Module.IsValid() && Found != mModuleInstanceDataOffsets.end() && mModuleInstancePayload != nullptr ? mModuleInstancePayload.get() + Found->second : nullptr;
}

/**
 * @brief 이 인스턴스에서 모듈에 배정한 실행 상태 영역을 읽기 전용으로 반환합니다.
 */
const Uint8* FParticleEmitterInstance::GetModuleInstanceData(CoreUObject::TObjectRef<const UParticleModule> Module) const {
    const auto Found{mModuleInstanceDataOffsets.find(Module)};

    return Module.IsValid() && Found != mModuleInstanceDataOffsets.end() && mModuleInstancePayload != nullptr ? mModuleInstancePayload.get() + Found->second : nullptr;
}

/**
 * @brief 이 인스턴스에 준비한 모듈 실행 데이터 버퍼의 크기를 반환합니다.
 */
Uint32 FParticleEmitterInstance::GetInstancePayloadSize() const {
    return mInstancePayloadSize;
}

/**
 * @brief 원본 핸들을 유지하면서 파티클과 모듈 실행 메모리 및 배치 스냅샷을 비웁니다.
 */
void FParticleEmitterInstance::ResetRuntimeData() {
    ClearParticles();

    mParticleData.reset();
    mParticleStride = 0;
    mParticleCapacity = 0;
    mModuleInstancePayload.reset();
    mInstancePayloadSize = 0;
    mModuleDataOffsets.clear();
    mModuleInstanceDataOffsets.clear();
    mModules.clear();
    mLODModules.clear();
    mLODLevels.clear();
    mCurrentLODIndex = -1;
    mEmitterToSimulation = FMatrix{};
    mSimulationToWorld = FMatrix{};
}
