/**
 * @file FParticleEmitterInstance.cpp
 * @brief 인스턴스의 원본 참조와 정렬된 모듈 실행 데이터 버퍼의 초기화·해제를 구현합니다.
 * @ingroup Cascade
 */

#include "Core/Types.h"
#include "Cascade/FParticleEmitterInstance.h"
#include "Cascade/UParticleEmitter.h"
#include "Cascade/UParticleModule.h"
#include "CoreUObject/Reflection/Cast.h"

#include <cstring>
#include <new>

using namespace Cascade;
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
 * @brief 인스턴스 페이로드와 모듈 정의에 대한 소유 참조를 해제합니다.
 */
FParticleEmitterInstance::~FParticleEmitterInstance() = default;

/**
 * @brief 원본 Emitter 참조를 교체하고 기존 페이로드와 배치 스냅샷을 비웁니다.
 */
void FParticleEmitterInstance::InitParameters(UParticleEmitter* Template) {
    mTemplate = CoreUObject::FObjectReference{Template};
    mModules.clear();
    mModuleDataOffsets.clear();
    mModuleInstanceDataOffsets.clear();
    mInstanceData.reset();
    mInstancePayloadSize = 0;
}

/**
 * @brief 원본 정의의 배치를 복사하고 모듈별 인스턴스 페이로드를 초기화합니다.
 */
void FParticleEmitterInstance::Init() {
    auto* Template{GetTemplate()};

    if (Template == nullptr) {
        return;
    }

    Template->Build();
    mModules = Template->GetModules();
    mModuleDataOffsets = Template->GetModuleDataOffsets();
    mModuleInstanceDataOffsets = Template->GetModuleInstanceDataOffsets();
    mInstancePayloadSize = Template->GetInstancePayloadSize();
    mInstanceData.reset();

    if (mInstancePayloadSize != 0) {
        mInstanceData.reset(static_cast<Uint8*>(::operator new(mInstancePayloadSize, std::align_val_t{16})));
        std::memset(mInstanceData.get(), 0, mInstancePayloadSize);
    }

    for (const auto& Module : mModules) {
        if (auto* Data{GetModuleInstanceData(Module.get())}; Data != nullptr) {
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
    return CoreUObject::Cast<UParticleEmitter>(mTemplate.Resolve());
}

/**
 * @brief 배치 스냅샷에서 모듈의 파티클 추가 데이터 오프셋을 조회합니다.
 */
Core::Index_t FParticleEmitterInstance::GetModuleDataOffset(const UParticleModule* Module) const {
    const auto Found{mModuleDataOffsets.find(Module)};

    return Found != mModuleDataOffsets.end() ? Found->second : 0;
}

/**
 * @brief 이 인스턴스에서 모듈에 배정한 실행 상태 영역을 반환합니다.
 */
Uint8* FParticleEmitterInstance::GetModuleInstanceData(const UParticleModule* Module) {
    const auto Found{mModuleInstanceDataOffsets.find(Module)};

    return Found != mModuleInstanceDataOffsets.end() && mInstanceData != nullptr ? mInstanceData.get() + Found->second : nullptr;
}

/**
 * @brief 이 인스턴스에서 모듈에 배정한 실행 상태 영역을 읽기 전용으로 반환합니다.
 */
const Uint8* FParticleEmitterInstance::GetModuleInstanceData(const UParticleModule* Module) const {
    const auto Found{mModuleInstanceDataOffsets.find(Module)};

    return Found != mModuleInstanceDataOffsets.end() && mInstanceData != nullptr ? mInstanceData.get() + Found->second : nullptr;
}

/**
 * @brief 이 인스턴스에 준비한 모듈 실행 데이터 버퍼의 크기를 반환합니다.
 */
Uint32 FParticleEmitterInstance::GetInstancePayloadSize() const {
    return mInstancePayloadSize;
}
