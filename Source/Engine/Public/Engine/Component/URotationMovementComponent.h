/**
 * @file URotationMovementComponent.h
 * @brief Game과 PIE 월드에서 소유 액터의 루트를 초당 회전 속도에 따라 로컬 또는 월드 공간으로 회전합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UActorComponent.h"
#include "Core/Math/FRotator.h"
#include "URotationMovementComponent.generated.h"

namespace Engine {

    /**
     * @brief Game과 PIE 월드에서 소유 액터의 루트를 초당 회전 속도에 따라 로컬 또는 월드 공간으로 회전합니다.
     */
    UCLASS()
    class URotationMovementComponent final : public Engine::UActorComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 기본 설정으로 URotationMovementComponent 객체를 초기화합니다.
         */
        URotationMovementComponent();
        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~URotationMovementComponent() override;

    public:
        /**
         * @brief Game 또는 PIE의 실행 시작 상태를 확인한 뒤 축별 속도로 소유 액터의 루트 회전을 갱신합니다.
         * @param[in] DeltaTime 이번 갱신에서 적분할 유한한 초 단위 시간입니다.
         */
        void Tick(float DeltaTime) override;

        /**
         * @brief 축별 초당 회전 속도를 반환합니다.
         * @return Pitch·Yaw·Roll의 도/초 값을 가진 읽기 전용 참조입니다.
         */
        const Core::FRotator& GetRotationRate() const;
        /**
         * @brief 유한한 Pitch·Yaw·Roll 회전 속도를 설정합니다.
         * @param[in] Rate Pitch·Yaw·Roll의 유한한 도/초 회전 속도입니다.
         * @return 모든 축이 유한하면 true이며 그렇지 않으면 false입니다.
         */
        bool SetRotationRate(const Core::FRotator& Rate);

        /**
         * @brief 회전 증분을 로컬 축으로 합성하는지 조회합니다.
         * @return 로컬 공간이면 true이며 월드 공간이면 false입니다.
         */
        bool GetRotationInLocalSpace() const;
        /**
         * @brief 회전 증분을 로컬 축 또는 월드 축으로 합성하도록 설정합니다.
         * @param[in] LocalSpace 로컬 공간을 사용하면 true이며 월드 공간을 사용하면 false입니다.
         * @return 설정을 반영하면 항상 true입니다.
         */
        bool SetRotationInLocalSpace(bool LocalSpace);

    private:
        /**
         * @brief Pitch·Yaw·Roll 축별 초당 회전 각도이며 단위는 도입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetRotationRate, Setter = SetRotationRate)
        Core::FRotator mRotationRate{0.0f, 90.0f, 0.0f};
        /**
         * @brief true이면 루트의 로컬 축, false이면 월드 축에 회전 증분을 합성합니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetRotationInLocalSpace, Setter = SetRotationInLocalSpace)
        bool mRotationInLocalSpace{true};
    };

}
