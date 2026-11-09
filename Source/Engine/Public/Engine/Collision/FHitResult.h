/**
 * @file FHitResult.h
 * @brief 광선 또는 도형 추적의 적중 위치·법선과 비소유 객체 참조입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FVector.h"

#include <cfloat>

namespace Engine {

    class UPrimitiveComponent;
    class AActor;

    /**
     * @brief 광선 또는 도형 추적의 적중 위치·법선과 비소유 객체 참조입니다.
     */
    struct FHitResult {
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
         * @brief 추적 시작·끝 위치와 빈 충돌 결과를 초기화합니다.
         */
        void Init();

        /**
         * @brief 추적 시작·끝 위치와 빈 충돌 결과를 초기화합니다.
         * @param[in] Start 추적 시작점입니다.
         * @param[in] End 추적 끝점입니다.
         */
        void Init(const Core::FVector& Start, const Core::FVector& End);

        /**
         * @brief 적중한 액터의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::AActor* GetActor() const;

        /**
         * @brief 적중한 프리미티브 컴포넌트의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::UPrimitiveComponent* GetComponent() const;

        /**
         * @brief 상대 형상 관점으로 법선과 소유 대상을 반전한 충돌 결과를 만듭니다.
         * @param[in] Hit 원본 충돌 결과입니다.
         * @param[in] MovedComponent 반전 결과에서 이동 주체로 기록할 컴포넌트입니다.
         * @param[in] MovedActor 반전 결과에서 이동 주체로 기록할 액터입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        static Engine::FHitResult GetReversedHit(const Engine::FHitResult& Hit, Engine::UPrimitiveComponent* MovedComponent, Engine::AActor* MovedActor);

    public:
        /**
         * @brief 충돌한 삼각형의 인덱스입니다.
         */
        Core::Int32 mFaceIndex{-1};

        /**
         * @brief 전체 추적 거리에서 적중한 비율입니다.
         */
        float mTime{1.0f};

        /**
         * @brief 추적 시작점부터 적중점까지의 거리입니다.
         */
        float mDistance{FLT_MAX};

        /**
         * @brief 충돌 시 이동 도형의 위치입니다.
         */
        Core::FVector mLocation{};

        /**
         * @brief 표면 위 실제 충돌점입니다.
         */
        Core::FVector mImpactPoint{};

        /**
         * @brief 충돌면의 단위 법선입니다.
         */
        Core::FVector mNormal{};

        /**
         * @brief 적중 표면의 단위 법선입니다.
         */
        Core::FVector mImpactNormal{};

        /**
         * @brief 추적 시작 위치입니다.
         */
        Core::FVector mTraceStart{};

        /**
         * @brief 추적 끝 위치입니다.
         */
        Core::FVector mTraceEnd{};

        /**
         * @brief 초기 겹침을 해소할 침투 깊이입니다.
         */
        float mPenetrationDepth{};

        /**
         * @brief 이동을 차단하는 충돌 여부입니다.
         */
        bool mBlockingHit{};

        /**
         * @brief 추적 시작부터 겹친 상태입니다.
         */
        bool mStartPenetrating{};

        /**
         * @brief 적중 컴포넌트의 비소유 주소입니다.
         */
        Engine::UPrimitiveComponent* mHitComponent{};

        /**
         * @brief 적중 액터의 비소유 주소입니다.
         */
        Engine::AActor* mHitActor{};
    };

}
