/**
 * @file IParticleEmitterInstanceOwner.cpp
 * @brief 인스턴스 소유자 인터페이스의 다형적 소멸을 구현합니다.
 * @ingroup Cascade
 */

#include "Engine/Cascade/IParticleEmitterInstanceOwner.h"

using namespace Engine;

/**
 * @brief 인스턴스 소유자 인터페이스의 다형적 소멸을 지원합니다.
 */
IParticleEmitterInstanceOwner::~IParticleEmitterInstanceOwner() = default;
