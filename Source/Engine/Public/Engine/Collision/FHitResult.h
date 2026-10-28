/**
 * @file FHitResult.h
 * @brief 광선 또는 도형 추적의 적중 위치·법선과 비소유 객체 참조입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FVector.h"

#include <cfloat>

class UPrimitiveComponent;
class AActor;

/** @brief 광선 또는 도형 추적의 적중 위치·법선과 비소유 객체 참조입니다. */
struct FHitResult {
public:
    ReflectTypes(FHitResult);

public:
    /**
     * @brief 전달된 설정과 기본값으로 FHitResult 객체를 생성합니다.
     */
    FHitResult() = default;
    /**
     * @brief 전달된 설정과 기본값으로 FHitResult 객체를 생성합니다.
     * @param[in] InTime 전체 추적 거리에서 적중한 비율입니다.
     */
    explicit FHitResult(float InTime);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FHitResult);
    /**
     * @brief 추적 시작·끝 위치와 빈 충돌 결과를 초기화합니다.
     */
    void Init();
    /**
     * @brief 추적 시작·끝 위치와 빈 충돌 결과를 초기화합니다.
     * @param[in] Start 추적 또는 선분의 시작점입니다.
     * @param[in] End 추적 또는 선분의 끝점입니다.
     */
    void Init(const FVector& Start, const FVector& End);
    /**
     * @brief 적중한 액터의 비소유 주소를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    AActor* GetActor() const;
    /**
     * @brief 적중한 프리미티브 컴포넌트의 비소유 주소를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    UPrimitiveComponent* GetComponent() const;
    /**
     * @brief 상대 형상 관점으로 법선과 소유 대상을 반전한 충돌 결과를 만듭니다.
     * @param[in] Hit 원본 충돌 결과입니다.
     * @param[in] MovedComponent 반전 결과에서 이동 주체로 기록할 컴포넌트입니다.
     * @param[in] MovedActor 반전 결과에서 이동 주체로 기록할 액터입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    static FHitResult GetReversedHit(const FHitResult& Hit, UPrimitiveComponent* MovedComponent, AActor* MovedActor);

private:
    ReflectMembers();

public:
    /**
     * @brief 충돌한 삼각형의 인덱스입니다.
     */
    Int32 mFaceIndex{-1};
    ReflectProperty(mFaceIndex, "FaceIndex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 전체 추적 거리에서 적중한 비율입니다.
     */
    float mTime{1.0f};
    ReflectProperty(mTime, "Time", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 추적 시작점부터 적중점까지의 거리입니다.
     */
    float mDistance{FLT_MAX};
    ReflectProperty(mDistance, "Distance", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 충돌 시 이동 도형의 위치입니다.
     */
    FVector mLocation{};
    ReflectProperty(mLocation, "Location", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 표면 위 실제 충돌점입니다.
     */
    FVector mImpactPoint{};
    ReflectProperty(mImpactPoint, "ImpactPoint", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 충돌면의 단위 법선입니다.
     */
    FVector mNormal{};
    ReflectProperty(mNormal, "Normal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 적중 표면의 단위 법선입니다.
     */
    FVector mImpactNormal{};
    ReflectProperty(mImpactNormal, "ImpactNormal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 추적 시작 위치입니다.
     */
    FVector mTraceStart{};
    ReflectProperty(mTraceStart, "TraceStart", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 추적 끝 위치입니다.
     */
    FVector mTraceEnd{};
    ReflectProperty(mTraceEnd, "TraceEnd", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 초기 겹침을 해소할 침투 깊이입니다.
     */
    float mPenetrationDepth{};
    ReflectProperty(mPenetrationDepth, "PenetrationDepth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 이동을 차단하는 충돌 여부입니다.
     */
    bool mBlockingHit{};
    ReflectProperty(mBlockingHit, "BlockingHit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 추적 시작부터 겹친 상태입니다.
     */
    bool mStartPenetrating{};
    ReflectProperty(mStartPenetrating, "StartPenetrating", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 적중 컴포넌트의 비소유 주소입니다.
     */
    UPrimitiveComponent* mHitComponent{};
    ReflectProperty(mHitComponent, "HitComponent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 적중 액터의 비소유 주소입니다.
     */
    AActor* mHitActor{};
    ReflectProperty(mHitActor, "HitActor", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
