/**
 * @file UParticleEmitter.cpp
 * @brief Emitter의 모듈 분류·메모리 배치·인스턴스 생성과 정의 직렬화를 구현합니다.
 * @ingroup Cascade
 */

#include "Core/Types.h"
#include "Cascade/UParticleEmitter.h"
#include "Cascade/FBaseParticle.h"
#include "Cascade/FParticleEmitterInstance.h"
#include "Cascade/UParticleModule.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Archive.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace Cascade;
using namespace Core;
using namespace CoreUObject;

/**
 * @brief 이 소스 파일 안에서만 사용하는 직렬화와 메모리 배치 보조 처리를 묶습니다.
 */
namespace {

    /**
     * @brief Emitter 하나가 보관할 수 있는 모듈 정의의 최대 개수입니다.
     */
    constexpr Uint32 MaximumModules{256};

    /**
     * @brief 주어진 바이트 수를 16바이트 경계로 올림합니다.
     * @param[in] Size 정렬할 바이트 수입니다.
     * @return 정렬 패딩을 포함한 바이트 수입니다.
     * @throws std::length_error 정렬 결과가 지원 오프셋 범위를 벗어나는 경우입니다.
     */
    Uint32 AlignPayload(Uint32 Size) {
        constexpr Uint32 MaximumSize{static_cast<Uint32>(std::numeric_limits<Int32>::max())};

        if (Size > MaximumSize - 15) {
            throw std::length_error{"Particle payload exceeds the supported offset range"};
        }

        return (Size + 15) & ~Uint32{15};
    }

    /**
     * @brief 버퍼 끝에 정렬된 모듈 영역을 배치하고 전체 크기를 늘립니다.
     * @param[in,out] Size 배치 전후의 전체 버퍼 크기입니다.
     * @param[in] RequiredSize 모듈에 필요한 추가 바이트 수입니다.
     * @return 배정한 영역의 시작 바이트 오프셋입니다.
     * @throws std::length_error 영역 끝이 지원 오프셋 범위를 벗어나는 경우입니다.
     */
    Core::Index_t AllocatePayload(Uint32& Size, Uint32 RequiredSize) {
        const Core::Index_t Offset{static_cast<Core::Index_t>(AlignPayload(Size))};

        if (RequiredSize > static_cast<Uint32>(std::numeric_limits<Int32>::max()) - Offset) {
            throw std::length_error{"Particle payload exceeds the supported offset range"};
        }

        Size = Offset + RequiredSize;

        return Offset;
    }

    /**
     * @brief 모듈 클래스 이름과 Parameters 객체를 읽거나 기록합니다.
     * @param[in,out] Archive 모듈 레코드에 진입한 로드 또는 저장 아카이브입니다.
     * @param[in,out] Module 저장할 모듈이며 로드 시 UClass 팩토리로 만든 객체를 받습니다.
     * @details 모듈의 정적 정의만 처리하며 파티클·인스턴스 실행 페이로드는 포함하지 않습니다.
     */
    void SerializeModule(FArchive& Archive, std::shared_ptr<UParticleModule>& Module) {
        FString Type{Archive.IsLoading() ? FString{} : Module->GetClass()->GetName()};

        Archive.Serialize("Type", Type);

        if (Archive.HasError()) {
            return;
        }

        if (Archive.IsLoading()) {
            const UClass* BaseClass{UParticleModule::StaticClass()};
            const UClass* Class{FReflectionRegistry::FindClass(Type)};

            if (Class == nullptr || !Class->IsChildOf(BaseClass)) {
                Archive.SetError("Unknown particle module class: " + Type);

                return;
            }

            std::shared_ptr<UObject> Object{Class->CreateInstance()};

            if (Cast<UParticleModule>(Object.get()) == nullptr) {
                Archive.SetError("Cannot create particle module class: " + Type);

                return;
            }

            Module = std::static_pointer_cast<UParticleModule>(Object);
        }

        if (Archive.BeginObject("Parameters")) {
            Module->Serialize(Archive);
            Archive.EndObject();
        }
    }

}

/**
 * @brief UObject 이름을 Emitter로 설정하고 빈 모듈 정의를 구성합니다.
 */
UParticleEmitter::UParticleEmitter() {
    SetName("Emitter");
}

/**
 * @brief 모듈의 Outer 연결과 Emitter가 가진 소유 참조 및 배치 캐시를 해제합니다.
 */
UParticleEmitter::~UParticleEmitter() {
    ClearModules();
}

/**
 * @brief 모듈을 자식으로 추가하고 실행 단계 목록을 갱신합니다.
 */
bool UParticleEmitter::AddModule(const std::shared_ptr<UParticleModule>& Module) {
    if (!Module || mModules.size() >= MaximumModules || std::find(mModules.begin(), mModules.end(), Module) != mModules.end()) {
        return false;
    }

    if ((Module->GetOuter() != nullptr && Module->GetOuter() != this) || !Module->SetOuter(this)) {
        return false;
    }

    mModules.push_back(Module);
    InvalidateBuildData();
    UpdateModuleLists();

    return true;
}

/**
 * @brief 모듈의 자식 연결을 제거하고 실행 단계 목록과 배치 캐시를 갱신합니다.
 */
bool UParticleEmitter::RemoveModule(const UParticleModule* Module) {
    for (auto Iterator{mModules.begin()}; Iterator != mModules.end(); ++Iterator) {
        if (Iterator->get() == Module) {
            if ((*Iterator)->GetOuter() == this) {
                (*Iterator)->SetOuter(nullptr);
            }

            mModules.erase(Iterator);
            InvalidateBuildData();
            UpdateModuleLists();

            return true;
        }
    }

    return false;
}

/**
 * @brief 모든 모듈의 Outer 연결과 소유 목록 및 실행 준비 정보를 비웁니다.
 */
void UParticleEmitter::ClearModules() {
    for (const auto& Module : mModules) {
        if (Module->GetOuter() == this) {
            Module->SetOuter(nullptr);
        }
    }

    mModules.clear();
    InvalidateBuildData();
    UpdateModuleLists();
}

/**
 * @brief 인덱스에 해당하는 모듈의 수정 가능한 비소유 주소를 반환합니다.
 */
UParticleModule* UParticleEmitter::GetModule(Core::Index_t Index) {
    return Index >= 0 && Index < mModules.size() ? mModules[Index].get() : nullptr;
}

/**
 * @brief 인덱스에 해당하는 모듈의 읽기 전용 비소유 주소를 반환합니다.
 */
const UParticleModule* UParticleEmitter::GetModule(Core::Index_t Index) const {
    return Index >= 0 && Index < mModules.size() ? mModules[Index].get() : nullptr;
}

/**
 * @brief Emitter에 등록된 모듈 정의의 개수를 반환합니다.
 */
std::size_t UParticleEmitter::GetModuleCount() const {
    return mModules.size();
}

/**
 * @brief 설정 순서를 유지하는 전체 모듈 소유 목록을 제공합니다.
 */
const TArray<std::shared_ptr<UParticleModule>>& UParticleEmitter::GetModules() const {
    return mModules;
}

/**
 * @brief 실행 단계별 모듈 목록과 파티클·인스턴스 페이로드 배치를 다시 구성합니다.
 */
void UParticleEmitter::Build() {
    UpdateModuleLists();
    CacheEmitterModuleInfo();
}

/**
 * @brief 모듈의 요구 바이트 수로 16바이트 정렬 오프셋과 전체 레코드 크기를 계산합니다.
 */
void UParticleEmitter::CacheEmitterModuleInfo() {
    TMap<const UParticleModule*, Core::Index_t> ParticleOffsets{};
    TMap<const UParticleModule*, Core::Index_t> InstanceOffsets{};
    Uint32 ParticleSize{static_cast<Uint32>(sizeof(FBaseParticle))};
    Uint32 InstanceSize{};

    for (const auto& Module : mModules) {
        const Uint32 ParticleBytes{Module->RequiredBytes(nullptr)};
        const Uint32 InstanceBytes{Module->RequiredBytesPerInstance()};

        if (ParticleBytes != 0) {
            ParticleOffsets.emplace(Module.get(), AllocatePayload(ParticleSize, ParticleBytes));
        }

        if (InstanceBytes != 0) {
            InstanceOffsets.emplace(Module.get(), AllocatePayload(InstanceSize, InstanceBytes));
        }
    }

    mParticleSize = AlignPayload(ParticleSize);
    mInstancePayloadSize = AlignPayload(InstanceSize);
    mModuleDataOffsets = std::move(ParticleOffsets);
    mModuleInstanceDataOffsets = std::move(InstanceOffsets);
}

/**
 * @brief 활성 모듈을 Spawn·Update·FinalUpdate 참여 플래그에 따라 순서대로 분류합니다.
 */
void UParticleEmitter::UpdateModuleLists() {
    mSpawnModules.clear();
    mUpdateModules.clear();
    mFinalUpdateModules.clear();

    for (const auto& Module : mModules) {
        if (!Module->mEnabled) {
            continue;
        }

        if (Module->mSpawnModule) {
            mSpawnModules.push_back(Module.get());
        }

        if (Module->mUpdateModule) {
            mUpdateModules.push_back(Module.get());
        }

        if (Module->mFinalUpdateModule) {
            mFinalUpdateModules.push_back(Module.get());
        }

    }
}

/**
 * @brief 이 정의를 참조하는 인스턴스를 생성하고 모듈 페이로드를 초기화합니다.
 */
std::unique_ptr<FParticleEmitterInstance> UParticleEmitter::CreateInstance(IParticleEmitterInstanceOwner& Owner) {
    auto Instance{std::make_unique<FParticleEmitterInstance>(Owner)};

    Instance->InitParameters(this);
    Instance->Init();

    return Instance;
}

/**
 * @brief 새 파티클 초기화 단계에 참여하는 활성 모듈 목록을 반환합니다.
 */
const TArray<UParticleModule*>& UParticleEmitter::GetSpawnModules() const {
    return mSpawnModules;
}

/**
 * @brief 파티클 갱신 단계에 참여하는 활성 모듈 목록을 반환합니다.
 */
const TArray<UParticleModule*>& UParticleEmitter::GetUpdateModules() const {
    return mUpdateModules;
}

/**
 * @brief 최종 갱신 단계에 참여하는 활성 모듈 목록을 반환합니다.
 */
const TArray<UParticleModule*>& UParticleEmitter::GetFinalUpdateModules() const {
    return mFinalUpdateModules;
}

/**
 * @brief 기본 파티클과 모듈 추가 데이터 및 패딩을 합친 레코드 크기를 반환합니다.
 */
Uint32 UParticleEmitter::GetParticleSize() const {
    return mParticleSize;
}

/**
 * @brief 인스턴스 하나에 필요한 전체 모듈 페이로드 크기를 반환합니다.
 */
Uint32 UParticleEmitter::GetInstancePayloadSize() const {
    return mInstancePayloadSize;
}

/**
 * @brief 모듈별 파티클 추가 데이터의 시작 오프셋 대응표를 반환합니다.
 */
const TMap<const UParticleModule*, Core::Index_t>& UParticleEmitter::GetModuleDataOffsets() const {
    return mModuleDataOffsets;
}

/**
 * @brief 모듈별 인스턴스 추가 데이터의 시작 오프셋 대응표를 반환합니다.
 */
const TMap<const UParticleModule*, Core::Index_t>& UParticleEmitter::GetModuleInstanceDataOffsets() const {
    return mModuleInstanceDataOffsets;
}

/**
 * @brief 모듈 구성이 바뀌어 유효하지 않은 오프셋과 레코드 크기 캐시를 비웁니다.
 */
void UParticleEmitter::InvalidateBuildData() {
    mModuleDataOffsets.clear();
    mModuleInstanceDataOffsets.clear();
    mParticleSize = 0;
    mInstancePayloadSize = 0;
}

/**
 * @brief Emitter 이름과 모듈 클래스·설정을 아카이브로 읽거나 기록합니다.
 */
void UParticleEmitter::Serialize(FArchive& Archive) {
    if (Archive.HasError()) {
        return;
    }

    FString Name{GetName()};

    Archive.Serialize("Name", Name);

    Uint32 Count{static_cast<Uint32>(mModules.size())};

    if (!Archive.BeginArray("Modules", Count)) {
        return;
    }

    if (Count > MaximumModules) {
        Archive.SetError("Too many particle modules");
    }

    TArray<std::shared_ptr<UParticleModule>> Modules{};

    for (Core::Index_t Index{}; Index < Count && !Archive.HasError(); ++Index) {
        if (!Archive.BeginObject(std::to_string(Index))) {
            break;
        }

        std::shared_ptr<UParticleModule> Module{Archive.IsLoading() ? nullptr : mModules[Index]};

        SerializeModule(Archive, Module);
        Archive.EndObject();

        if (Archive.IsLoading() && !Archive.HasError()) {
            Modules.push_back(std::move(Module));
        }
    }

    Archive.EndArray();

    if (Archive.IsLoading() && !Archive.HasError()) {
        UParticleEmitter Candidate{};

        if (!Candidate.SetName(Name)) {
            Archive.SetError("Invalid particle emitter name");

            return;
        }

        Candidate.mModules = std::move(Modules);

        for (const auto& Module : Candidate.mModules) {
            Module->SetOuter(&Candidate);
        }

        try {
            Candidate.Build();
            ClearModules();
            SetName(Name);
            mModules = std::move(Candidate.mModules);
            mSpawnModules = std::move(Candidate.mSpawnModules);
            mUpdateModules = std::move(Candidate.mUpdateModules);
            mFinalUpdateModules = std::move(Candidate.mFinalUpdateModules);
            mModuleDataOffsets = std::move(Candidate.mModuleDataOffsets);
            mModuleInstanceDataOffsets = std::move(Candidate.mModuleInstanceDataOffsets);
            mParticleSize = Candidate.mParticleSize;
            mInstancePayloadSize = Candidate.mInstancePayloadSize;

            for (const auto& Module : mModules) {
                Module->SetOuter(this);
            }
        } catch (const std::exception& Error) {
            Archive.SetError(Error.what());
        }
    }
}
