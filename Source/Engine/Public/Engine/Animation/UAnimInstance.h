/**
 * @file UAnimInstance.h
 * @brief 스켈레탈 메시 컴포넌트의 애니메이션 상태를 갱신하고 본 포즈를 계산하는 객체입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Math/FTransform.h"
#include "CoreUObject/UObject.h"
#include "UAnimInstance.generated.h"

namespace Asset {
    class UAnimSequence;
    class USkeleton;
}

namespace Engine {

    class USkeletalMeshComponent;

    /**
     * @brief 컴포넌트 하나의 애니메이션 변수와 포즈 계산을 맡습니다.
     * @details 언리얼 UAnimInstance와 같이 컴포넌트가 매 Tick UpdateAnimation을 호출하면 NativeUpdateAnimation으로 상태를 갱신하고,
     * NativeEvaluateAnimation으로 본 인덱스 순서의 로컬 포즈를 만듭니다. 포즈는 스켈레톤의 레퍼런스 포즈(USkeleton::GetReferencePose)에서 시작하며, 기본 구현은 그대로 출력합니다.
     *
     * 애니메이션 시퀀스는 스켈레톤을 참조하지 않고 트랙을 본 이름으로 가리키므로, 시퀀스 트랙을 스켈레톤 본 인덱스로 바꾸는 표를 만들어 둡니다.
     * 스켈레톤에 없는 트랙은 무시하고, 트랙이 없는 본은 레퍼런스 포즈를 유지합니다.
     *
     * 스켈레톤은 저장하지 않고 소유 컴포넌트에서 조회합니다. 컴포넌트가 메시나 스켈레톤이 바뀐 것을 감지하면 InitializeAnimation을 다시 호출하고,
     * 이때 매핑 표를 비웁니다. 평가할 시퀀스를 바꿀 때는 파생 클래스가 ResetTrackMapping으로 표를 비웁니다.
     */
    UCLASS()
    class UAnimInstance : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        UAnimInstance();

        ~UAnimInstance() override;

    public:
        /**
         * @brief 소유 컴포넌트를 연결하고 매핑 표를 비운 뒤 NativeInitializeAnimation을 호출합니다.
         * @details 컴포넌트의 메시나 스켈레톤이 바뀔 때마다 다시 호출되며, 재생 위치 같은 파생 클래스의 상태는 유지합니다.
         * @param[in] Owner 이 인스턴스를 소유하는 컴포넌트입니다.
         */
        void InitializeAnimation(Engine::USkeletalMeshComponent* Owner);

        /**
         * @brief 상태를 갱신한 뒤 현재 포즈를 계산합니다.
         * @param[in] DeltaSeconds 지난 프레임 이후 경과 시간입니다.
         */
        void UpdateAnimation(float DeltaSeconds);

        /**
         * @brief 마지막으로 계산한 포즈를 반환합니다.
         * @return 스켈레톤 본 인덱스 순서의 부모 본 기준 변환이며, 스켈레톤이 없으면 비어 있습니다.
         */
        const Core::TArray<Core::FTransform>& GetPose() const;

        /**
         * @brief 소유 컴포넌트를 반환합니다.
         * @return 초기화하지 않았으면 nullptr입니다.
         */
        Engine::USkeletalMeshComponent* GetOwningComponent() const;

    protected:
        /**
         * @brief 초기화할 때 한 번 호출됩니다.
         */
        virtual void NativeInitializeAnimation();

        /**
         * @brief 매 프레임 포즈 계산 전에 호출되어 재생 시간 같은 상태를 갱신합니다.
         * @param[in] DeltaSeconds 지난 프레임 이후 경과 시간입니다.
         */
        virtual void NativeUpdateAnimation(float DeltaSeconds);

        /**
         * @brief 레퍼런스 포즈로 채운 포즈를 받아 이번 프레임의 포즈로 바꿉니다.
         * @param[in,out] Pose 스켈레톤 본 인덱스 순서의 로컬 변환입니다.
         */
        virtual void NativeEvaluateAnimation(Core::TArray<Core::FTransform>& Pose);

        /**
         * @brief 시퀀스를 지정한 시간으로 샘플링해 트랙이 있는 본의 변환을 덮어씁니다.
         * @param[in] Sequence 샘플링할 시퀀스입니다.
         * @param[in] Time 초 단위 재생 시간입니다.
         * @param[in] Looping 반복 재생 여부입니다.
         * @param[in,out] Pose 스켈레톤 본 인덱스 순서의 로컬 변환입니다.
         */
        void EvaluateSequence(const Asset::UAnimSequence& Sequence, double Time, bool Looping, Core::TArray<Core::FTransform>& Pose);

        /**
         * @brief 트랙 → 본 매핑 표를 비워 다음 EvaluateSequence가 새로 만들게 합니다.
         * @details 평가할 시퀀스를 바꿀 때 호출합니다. 스켈레톤이 바뀔 때는 InitializeAnimation이 호출합니다.
         */
        void ResetTrackMapping();

        /**
         * @brief 포즈 계산에 쓰는 스켈레톤을 반환합니다.
         * @return 소유 컴포넌트의 현재 스켈레톤이며 없으면 nullptr입니다.
         */
        const Asset::USkeleton* GetSkeleton() const;

    private:
        /**
         * @brief 이 인스턴스를 소유하는 컴포넌트입니다. 컴포넌트가 인스턴스보다 오래 살아 있습니다.
         */
        Engine::USkeletalMeshComponent* mOwner{};

        /**
         * @brief 시퀀스 트랙 인덱스별 스켈레톤 본 인덱스이며 스켈레톤에 없는 트랙은 -1입니다.
         * @details 비어 있으면 다음 EvaluateSequence에서 새로 만듭니다. 시퀀스나 스켈레톤이 바뀌면 ResetTrackMapping으로 비웁니다.
         */
        Core::TArray<Core::Int32> mTrackToBone{};

        /**
         * @brief 샘플링 결과를 담는 트랙 순서의 임시 포즈입니다.
         */
        Core::TArray<Core::FTransform> mTrackPose{};

        /**
         * @brief 마지막으로 계산한 본 인덱스 순서의 포즈입니다.
         */
        Core::TArray<Core::FTransform> mPose{};
    };

}
