/**
 * @file FBaseParticle.cpp
 * @brief 파티클 기본 속성의 초기 상태와 읽기·수정 접근 함수를 구현합니다.
 * @ingroup Cascade
 */

#include "Engine/Cascade/FBaseParticle.h"

using namespace Engine;
using namespace Core;

/**
 * @brief 위치·속도·수명·회전은 0, 크기와 RGBA 색상 성분은 1인 기본 파티클을 구성합니다.
 */
FBaseParticle::FBaseParticle() = default;

/**
 * @brief 파티클의 현재 위치 속성을 직접 수정할 수 있도록 반환합니다.
 */
FVector& FBaseParticle::GetLocation() {
    return mLocation;
}

/**
 * @brief 파티클의 현재 위치 속성을 읽습니다.
 */
const FVector& FBaseParticle::GetLocation() const {
    return mLocation;
}

/**
 * @brief 파티클의 현재 속도 속성을 직접 수정할 수 있도록 반환합니다.
 */
FVector& FBaseParticle::GetVelocity() {
    return mVelocity;
}

/**
 * @brief 파티클의 현재 속도 속성을 읽습니다.
 */
const FVector& FBaseParticle::GetVelocity() const {
    return mVelocity;
}

/**
 * @brief 파티클의 축별 크기 속성을 직접 수정할 수 있도록 반환합니다.
 */
FVector& FBaseParticle::GetSize() {
    return mSize;
}

/**
 * @brief 파티클의 축별 크기 속성을 읽습니다.
 */
const FVector& FBaseParticle::GetSize() const {
    return mSize;
}

/**
 * @brief 파티클의 알파를 포함한 RGBA 색상 속성을 직접 수정할 수 있도록 반환합니다.
 */
FVector4& FBaseParticle::GetColor() {
    return mColor;
}

/**
 * @brief 파티클의 알파를 포함한 RGBA 색상 속성을 읽습니다.
 */
const FVector4& FBaseParticle::GetColor() const {
    return mColor;
}

/**
 * @brief 파티클의 수명 대비 정규화된 경과 시간 속성을 직접 수정할 수 있도록 반환합니다.
 */
float& FBaseParticle::GetRelativeTime() {
    return mRelativeTime;
}

/**
 * @brief 파티클의 수명 대비 정규화된 경과 시간 속성을 읽습니다.
 */
float FBaseParticle::GetRelativeTime() const {
    return mRelativeTime;
}

/**
 * @brief 파티클의 최대 수명의 역수 속성을 직접 수정할 수 있도록 반환합니다.
 */
float& FBaseParticle::GetOneOverMaxLifetime() {
    return mOneOverMaxLifetime;
}

/**
 * @brief 파티클의 최대 수명의 역수 속성을 읽습니다.
 */
float FBaseParticle::GetOneOverMaxLifetime() const {
    return mOneOverMaxLifetime;
}

/**
 * @brief 파티클의 현재 회전값 속성을 직접 수정할 수 있도록 반환합니다.
 */
float& FBaseParticle::GetRotation() {
    return mRotation;
}

/**
 * @brief 파티클의 현재 회전값 속성을 읽습니다.
 */
float FBaseParticle::GetRotation() const {
    return mRotation;
}

/**
 * @brief 파티클의 현재 회전 속도 속성을 직접 수정할 수 있도록 반환합니다.
 */
float& FBaseParticle::GetRotationRate() {
    return mRotationRate;
}

/**
 * @brief 파티클의 현재 회전 속도 속성을 읽습니다.
 */
float FBaseParticle::GetRotationRate() const {
    return mRotationRate;
}
