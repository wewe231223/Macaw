/**
 * @file FHitResult.cpp
 * @brief 광선 또는 도형 추적의 적중 위치·법선과 비소유 객체 참조입니다.
 * @ingroup Engine
 */

#include "Engine/Collision/FHitResult.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/AActor.h"
#include "Engine/Component/UPrimitiveComponent.h"

namespace Engine {

    /**
     * @brief 전달된 설정과 기본값으로 FHitResult 객체를 생성합니다.
     */
    Engine::FHitResult::FHitResult(float InTime)
		: mTime{InTime} {
    }

    /**
     * @brief 추적 시작·끝 위치와 빈 충돌 결과를 초기화합니다.
     */
    void Engine::FHitResult::Init() {
        *this = Engine::FHitResult{};
    }

    /**
     * @brief 추적 시작·끝 위치와 빈 충돌 결과를 초기화합니다.
     */
    void Engine::FHitResult::Init(const Core::FVector& Start, const Core::FVector& End) {
        Init();

        mTraceStart = Start;
        mTraceEnd = End;
    }

    /**
     * @brief 적중한 액터의 비소유 주소를 반환합니다.
     */
    Engine::AActor* Engine::FHitResult::GetActor() const {
        Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(mHitActor.Get())};

        return Actor != nullptr && !Actor->IsPendingDestroy() ? Actor : nullptr;
    }

    /**
     * @brief 적중한 프리미티브 컴포넌트의 비소유 주소를 반환합니다.
     */
    Engine::UPrimitiveComponent* Engine::FHitResult::GetComponent() const {
        Engine::UPrimitiveComponent* Component{CoreUObject::Cast<Engine::UPrimitiveComponent>(mHitComponent.Get())};
        const Engine::AActor* Owner{Component != nullptr ? Component->GetOwner() : nullptr};

        return Owner == nullptr || !Owner->IsPendingDestroy() ? Component : nullptr;
    }

    void Engine::FHitResult::SetActor(Engine::AActor* Actor) {
        mHitActor = Actor;
    }

    void Engine::FHitResult::SetComponent(Engine::UPrimitiveComponent* Component) {
        mHitComponent = Component;
    }

    /**
     * @brief 상대 형상 관점으로 법선과 소유 대상을 반전한 충돌 결과를 만듭니다.
     */
    Engine::FHitResult Engine::FHitResult::GetReversedHit(const Engine::FHitResult& Hit, Engine::UPrimitiveComponent* MovedComponent, Engine::AActor* MovedActor) {
        Engine::FHitResult Result{Hit};
        Result.mNormal = -Hit.mNormal;
        Result.mImpactNormal = -Hit.mImpactNormal;
        Result.SetComponent(MovedComponent);
        Result.SetActor(MovedActor);

        return Result;
    }

}
