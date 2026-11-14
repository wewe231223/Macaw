/**
 * @file UParticleModule.cpp
 * @brief 모듈 컨텍스트의 접근 함수와 기본 직렬화 및 생성·갱신 확장 지점을 구현합니다.
 * @ingroup Cascade
 */

#include "Core/Types.h"
#include "Cascade/UParticleModule.h"
#include "Cascade/UParticleEmitter.h"
#include "Cascade/FParticleEmitterInstance.h"
#include "Cascade/IParticleEmitterInstanceOwner.h"

using namespace Cascade;
using namespace Core;

/**
 * @brief 실행 인스턴스에 대한 비소유 참조로 공통 컨텍스트를 구성합니다.
 */
UParticleModule::FContext::FContext(FParticleEmitterInstance& Owner)
	: mOwner{Owner} {
}

/**
 * @brief 이 호출의 실행 상태를 보관하는 인스턴스를 반환합니다.
 */
FParticleEmitterInstance& UParticleModule::FContext::GetOwner() const {
    return mOwner;
}

/**
 * @brief 소유자가 제공하는 현재 공간 변환을 반환합니다.
 */
const FTransform& UParticleModule::FContext::GetTransform() const {
    return mOwner.GetOwner().GetTransform();
}

/**
 * @brief 실행 소유자가 제공하는 인스턴스 이름을 반환합니다.
 */
FString UParticleModule::FContext::GetInstanceName() const {
    return mOwner.GetOwner().GetInstanceName();
}

/**
 * @brief 인스턴스의 원본 Emitter 정의 이름을 반환합니다.
 */
FString UParticleModule::FContext::GetTemplateName() const {
    const auto* Template{mOwner.GetTemplate()};

    return Template != nullptr ? Template->GetName() : FString{};
}

/**
 * @brief 새 파티클 초기화 호출에 필요한 정보를 구성합니다.
 */
UParticleModule::FSpawnContext::FSpawnContext(FParticleEmitterInstance& Owner, Core::Index_t Offset, float SpawnTime, FBaseParticle* ParticleBase)
	: FContext{Owner},
	  mOffset{Offset},
	  mSpawnTime{SpawnTime},
	  mParticleBase{ParticleBase} {
}

/**
 * @brief 파티클 레코드 내 모듈 추가 데이터의 위치를 반환합니다.
 */
Core::Index_t UParticleModule::FSpawnContext::GetOffset() const {
    return mOffset;
}

/**
 * @brief 이번 생성 호출에 전달된 시간 값을 반환합니다.
 */
float UParticleModule::FSpawnContext::GetSpawnTime() const {
    return mSpawnTime;
}

/**
 * @brief 이번 호출에서 초기화할 기본 파티클 데이터를 반환합니다.
 */
FBaseParticle* UParticleModule::FSpawnContext::GetParticleBase() const {
    return mParticleBase;
}

/**
 * @brief 모듈 갱신 호출의 인스턴스와 데이터 위치 및 시간 간격을 구성합니다.
 */
UParticleModule::FUpdateContext::FUpdateContext(FParticleEmitterInstance& Owner, Core::Index_t Offset, float DeltaTime)
	: FContext{Owner},
	  mOffset{Offset},
	  mDeltaTime{DeltaTime} {
}

/**
 * @brief 파티클 레코드 내 모듈 추가 데이터의 위치를 반환합니다.
 */
Core::Index_t UParticleModule::FUpdateContext::GetOffset() const {
    return mOffset;
}

/**
 * @brief 이번 모듈 갱신에 전달된 시간 간격을 반환합니다.
 */
float UParticleModule::FUpdateContext::GetDeltaTime() const {
    return mDeltaTime;
}

/**
 * @brief 기본 모듈 이름과 활성·실행 단계 설정을 구성합니다.
 */
UParticleModule::UParticleModule() {
    SetName("ParticleModule");
}

/**
 * @brief 모듈 UObject의 수명을 종료합니다. 인스턴스 실행 메모리는 소유하지 않습니다.
 */
UParticleModule::~UParticleModule() = default;

/**
 * @brief UObject 식별 정보와 Serialize 프로퍼티로 등록된 모듈 설정을 직렬화합니다.
 */
void UParticleModule::Serialize(Core::FArchive& Archive) {
    CoreUObject::UObject::Serialize(Archive);
}

/**
 * @brief 이 모듈이 파티클 하나마다 요구하는 추가 데이터 크기를 반환합니다.
 */
Uint32 UParticleModule::RequiredBytes(UParticleModuleTypeDataBase*) {
    return 0;
}

/**
 * @brief 이 모듈이 EmitterInstance 하나마다 요구하는 추가 데이터 크기를 반환합니다.
 */
Uint32 UParticleModule::RequiredBytesPerInstance() {
    return 0;
}

/**
 * @brief 모듈에 할당된 인스턴스 페이로드를 초기화하는 확장 지점입니다.
 */
Uint32 UParticleModule::PrepPerInstanceBlock(FParticleEmitterInstance*, void*) {
    return 0;
}

/**
 * @brief 생성된 파티클 하나의 초기 속성을 설정하는 확장 지점입니다.
 */
void UParticleModule::Spawn(const FSpawnContext&) {
}

/**
 * @brief 인스턴스의 대상 파티클들을 모듈 단위로 갱신하는 확장 지점입니다.
 */
void UParticleModule::Update(const FUpdateContext&) {
}

/**
 * @brief 일반 갱신 뒤에 필요한 파티클 후처리를 수행하는 확장 지점입니다.
 */
void UParticleModule::FinalUpdate(const FUpdateContext&) {
}

