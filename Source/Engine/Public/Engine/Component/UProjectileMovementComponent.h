/**
 * @file UProjectileMovementComponent.h
 * @brief PIE 월드에서만 초기 속도·중력·최대 속도에 따라 소유 액터를 이동하는 발사체 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UActorComponent.h"
#include "Core/Math/FVector.h"

namespace Engine {

    /**
     * @brief PIE 월드에서만 초기 속도·중력·최대 속도에 따라 소유 액터를 이동하는 발사체 컴포넌트입니다.
     */
    class UProjectileMovementComponent final : public Engine::UActorComponent {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(UProjectileMovementComponent, Engine::UActorComponent);

    public:
        /**
         * @brief 기본 설정으로 UProjectileMovementComponent 객체를 초기화합니다.
         */
        UProjectileMovementComponent();
        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~UProjectileMovementComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UProjectileMovementComponent);

        /**
         * @brief 초기 속도를 필요 시 월드 공간으로 변환하고 최대 속도를 적용하여 정지 상태를 해제합니다.
         */
        void BeginPlay() override;
        /**
         * @brief PIE에서 중력·최대 속도를 반영하여 하위 시간 간격으로 적분하고 필요 시 속도 방향으로 회전합니다.
         * @param[in] DeltaTime 이번 갱신에서 적분할 유한한 초 단위 시간입니다.
         */
        void Tick(float DeltaTime) override;
        /**
         * @brief 실행 종료 시 속도를 0으로 만들고 발사체 이동을 정지합니다.
         */
        void EndPlay() override;
        /**
         * @brief 현재 속도를 0으로 만들고 다음 속도 설정 또는 실행 시작까지 이동을 정지합니다.
         */
        void StopMovement();

        /**
         * @brief 실행 시작에 사용할 초기 속도 설정을 반환합니다.
         * @return InitialVelocityInLocalSpace 설정에 따라 해석되는 월드 단위/초 속도의 읽기 전용 참조입니다.
         */
        const Core::FVector& GetInitialVelocity() const;
        /**
         * @brief 유한한 초기 속도를 저장하며 현재 실행 중인 속도는 즉시 바꾸지 않습니다.
         * @param[in] Velocity 유한한 월드 단위/초 속도 벡터입니다.
         * @return 모든 성분이 유한하면 true이며 그렇지 않으면 false입니다.
         */
        bool SetInitialVelocity(const Core::FVector& Velocity);

        /**
         * @brief 현재 월드 공간 발사체 속도를 조회합니다.
         * @return 컴포넌트가 보관하는 월드 단위/초 속도의 읽기 전용 참조입니다.
         */
        const Core::FVector& GetVelocity() const;
        /**
         * @brief 현재 월드 속도를 설정하고 최대 속도로 제한한 뒤 정지 상태를 해제합니다.
         * @param[in] Velocity 유한한 월드 단위/초 속도 벡터입니다.
         * @return 모든 성분이 유한하면 true이며 그렇지 않으면 false입니다.
         */
        bool SetVelocity(const Core::FVector& Velocity);

        /**
         * @brief 속도 크기의 상한을 반환합니다.
         * @return 월드 단위/초의 최대 속도이며 0이면 상한이 없습니다.
         */
        float GetMaxSpeed() const;
        /**
         * @brief 0 이상의 유한한 최대 속도를 저장하고 필요한 경우 현재 속도를 제한합니다.
         * @param[in] Speed 0 이상의 유한한 최대 속도이며 0이면 제한을 해제합니다.
         * @return 유효한 값을 적용하면 true이며 음수 또는 비유한 값이면 false입니다.
         */
        bool SetMaxSpeed(float Speed);

        /**
         * @brief -Z 방향 기본 중력 9.81에 곱할 배율을 반환합니다.
         * @return 중력 배율이며 0이면 중력 가속도가 없습니다.
         */
        float GetGravityScale() const;
        /**
         * @brief 0~100 범위의 유한한 중력 배율을 설정합니다.
         * @param[in] Scale 기본 중력 9.81에 곱할 유한한 0~100 배율입니다.
         * @return 유효한 배율을 적용하면 true이며 그렇지 않으면 false입니다.
         */
        bool SetGravityScale(float Scale);

        /**
         * @brief 초기 속도를 루트의 로컬 회전으로 변환하는지 조회합니다.
         * @return 초기 속도를 로컬 공간으로 해석하면 true입니다.
         */
        bool GetInitialVelocityInLocalSpace() const;
        /**
         * @brief 실행 시작 시 초기 속도를 로컬 또는 월드 공간으로 해석하도록 설정합니다.
         * @param[in] LocalSpace 로컬 공간을 사용하면 true이며 월드 공간을 사용하면 false입니다.
         * @return 설정을 반영하면 항상 true입니다.
         */
        bool SetInitialVelocityInLocalSpace(bool LocalSpace);

        /**
         * @brief 루트의 전방을 현재 속도 방향에 맞추는지 조회합니다.
         * @return 속도 방향으로 루트를 회전시키면 true입니다.
         */
        bool GetRotationFollowsVelocity() const;
        /**
         * @brief 이동 후 속도가 충분히 클 때 루트 회전이 속도 방향을 따르도록 설정합니다.
         * @param[in] FollowsVelocity 속도 방향으로 루트를 회전시킬지 나타냅니다.
         * @return 설정을 반영하면 항상 true입니다.
         */
        bool SetRotationFollowsVelocity(bool FollowsVelocity);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 게임 실행 시작 시 적용할 초기 속도이며 월드 단위/초입니다.
         */
        Core::FVector mInitialVelocity{10.0f, 0.0f, 5.0f};
        ReflectAccessor(mInitialVelocity, GetInitialVelocity, SetInitialVelocity, "InitialVelocity", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 발사체 속도 크기의 상한이며 0이면 제한하지 않습니다.
         */
        float mMaxSpeed{100.0f};
        ReflectAccessor(mMaxSpeed, GetMaxSpeed, SetMaxSpeed, "MaxSpeed", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 월드 -Z 방향의 기본 중력 9.81에 곱할 0~100 배율입니다.
         */
        float mGravityScale{1.0f};
        ReflectAccessor(mGravityScale, GetGravityScale, SetGravityScale, "GravityScale", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 실행 시작 시 초기 속도를 소유 액터의 루트 회전으로 월드 공간에 변환할지 나타냅니다.
         */
        bool mInitialVelocityInLocalSpace{true};
        ReflectAccessor(mInitialVelocityInLocalSpace, GetInitialVelocityInLocalSpace, SetInitialVelocityInLocalSpace, "InitialVelocityInLocalSpace", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 속도가 충분히 클 때 루트의 전방을 현재 속도 방향에 맞출지 나타냅니다.
         */
        bool mRotationFollowsVelocity{true};
        ReflectAccessor(mRotationFollowsVelocity, GetRotationFollowsVelocity, SetRotationFollowsVelocity, "RotationFollowsVelocity", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 현재 월드 공간 속도이며 실행 시작·중력·속도 제한에 따라 갱신됩니다.
         */
        Core::FVector mVelocity{};
        ReflectProperty(mVelocity, "Velocity", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정지 요청 또는 EndPlay 이후 이동 적분을 생략할지 나타냅니다.
         */
        bool mStopped{};
        ReflectProperty(mStopped, "Stopped", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
