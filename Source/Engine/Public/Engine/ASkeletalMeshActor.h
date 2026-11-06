/**
 * @file ASkeletalMeshActor.h
 * @brief 스켈레탈 메시 컴포넌트를 루트로 갖는 배치 가능한 액터입니다.
 * @ingroup Engine
 */
#pragma once

#include "Engine/AActor.h"
#include "ASkeletalMeshActor.generated.h"

namespace Engine {
    class USkeletalMeshComponent;

    /**
     * @brief 에디터와 씬 직렬화에서 생성할 수 있는 스켈레탈 메시 액터입니다.
     */
    UCLASS()
    class ASkeletalMeshActor final : public Engine::AActor {
        GENERATED_BODY()

    public:
        /**
         * @brief 빈 액터를 생성합니다.
         */
        ASkeletalMeshActor() = default;
        /**
         * @brief 소유 컴포넌트를 해제합니다.
         */
        ~ASkeletalMeshActor() override = default;

    public:
        /**
         * @brief 기본 스켈레탈 메시 컴포넌트를 추가합니다.
         * @return 초기화했으면 true입니다.
         */
        bool InitializeDefaultComponents() override;
        /**
         * @brief 루트 메시 컴포넌트를 반환합니다.
         * @return 루트가 해당 타입이 아니면 nullptr입니다.
         */
        Engine::USkeletalMeshComponent* GetSkeletalMeshComponent() const;

    };
}
