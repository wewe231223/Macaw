/**
 * @file UAnimInstance.h
 * @brief 스켈레탈 메시 컴포넌트의 애니메이션 상태를 갱신하고 본 포즈를 계산하는 객체입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FVector.h"
#include "CoreUObject/UObject.h"
#include "UAnimInstance.generated.h"

namespace Asset {
    class UAnimSequence;
    class UBlendSpace;
    class USkeleton;
}

namespace Engine {

    /**
     * @brief 시퀀스에서 루트 모션을 추출하고 소비자에게 전달하는 방식입니다.
     * @ingroup Engine
     */
    UENUM()
    enum class ERootMotionMode : Core::Uint8 {

        /**
         * @brief 루트 모션을 추출하지 않고 애니메이션 포즈를 유지합니다.
         */
        NoRootMotionExtraction,

        /**
         * @brief 루트 모션을 포즈에서 추출하되 액터 적용용으로 누적하지 않습니다.
         */
        IgnoreRootMotion,

        /**
         * @brief 평가한 포즈의 루트 모션을 블렌딩하고 소비용으로 누적합니다.
         */
        RootMotionFromEverything
    };

    class USkeletalMeshComponent;
    struct FAnimNode_Base;
    struct FAnimNodeValue;
    struct FPoseLink;
    struct FAnimationUpdateContext;

    /**
     * @brief 포즈 그래프 평가 중 방문한 노드, 부모 방문과 샘플별 가중치입니다.
     * @ingroup Engine
     */
    struct FAnimNodeVisit {
    public:

        /**
         * @brief 애니메이션 인스턴스가 소유하는 방문 포즈 노드입니다.
         */
        const Engine::FAnimNode_Base* mNode{};

        /**
         * @brief 방문에 사용한 포즈 연결의 비소유 포인터입니다.
         */
        const Engine::FPoseLink* mLink{};

        /**
         * @brief 최종 포즈에 기여하는 방문 가중치입니다.
         */
        float mWeight{};

        /**
         * @brief 부모 방문 인덱스이며 루트 방문은 -1입니다.
         */
        Core::Index_t mParentIndex{-1};

        /**
         * @brief 샘플 기반 노드의 샘플 인덱스이며 해당하지 않으면 -1입니다.
         */
        Core::Index_t mSampleIndex{-1};
    };

    /**
     * @brief 컴포넌트 하나의 애니메이션 변수와 노드 그래프를 소유하고 포즈를 계산합니다.
     * @details 언리얼 UAnimInstance와 같이 컴포넌트가 매 Tick UpdateAnimation을 호출하면 다음 순서로 진행합니다.
     * 1. NativeUpdateAnimation이 변수(예: 속도)를 갱신합니다.
     * 2. 루트 노드(mRootNode)부터 Update를 전파해 재생 시간과 블렌드 가중치를 갱신합니다.
     * 3. 스켈레톤의 레퍼런스 포즈에서 시작해 NativeEvaluateAnimation이 포즈를 계산합니다. 기본 구현은 루트 노드를 Evaluate합니다.
     *
     * 파생 클래스는 노드를 멤버로 두고 NativeInitializeAnimation에서 노드를 연결한 뒤 mRootNode를 지정합니다.
     * UPROPERTY(Edit, Serialize) 멤버는 컴포넌트에 저장되어 에셋 경로 같은 설정을 받습니다.
     *
     * 애니메이션 시퀀스는 스켈레톤을 참조하지 않고 트랙을 본 이름으로 가리키므로, 시퀀스마다 트랙을 스켈레톤 본 인덱스로 바꾸는 표를 만들어 둡니다.
     * 스켈레톤에 없는 트랙은 무시하고, 트랙이 없는 본은 레퍼런스 포즈를 유지합니다. 캐시된 시퀀스는 바뀌지 않으므로 표는 시퀀스 주소로 찾습니다.
     *
     * 스켈레톤은 저장하지 않고 소유 컴포넌트에서 조회합니다. 컴포넌트가 메시나 스켈레톤이 바뀐 것을 감지하면 InitializeAnimation을 다시 호출하고,
     * 이때 매핑 표를 비우고 그래프를 다시 만듭니다.
     */
    UCLASS()
    class UAnimInstance : public CoreUObject::UObject {
        GENERATED_BODY()

    private:

        /**
         * @brief 시퀀스 트랙을 현재 스켈레톤 본에 대응시키고 루트 모션 트랙을 캐시합니다.
         * @ingroup Engine
         */
        struct FSequenceBoneMapping {

            /**
             * @brief 시퀀스 트랙별 스켈레톤 본 인덱스이며 미대응 트랙은 -1입니다.
             */
            Core::TArray<Core::Index_t> mTrackToBone{};

            /**
             * @brief 루트 모션을 추출할 본 이름이며 빈 값은 기본 루트를 사용합니다.
             */
            Core::FString mRootMotionBone{};

            /**
             * @brief 루트 모션을 추출할 시퀀스 트랙 인덱스이며 없으면 -1입니다.
             */
            Core::Index_t mRootTrack{-1};

            /**
             * @brief 루트 모션 트랙이 대응하는 스켈레톤 본 인덱스이며 없으면 -1입니다.
             */
            Core::Index_t mRootBone{-1};
        };

    public:

        /**
         * @brief UAnimInstance의 초기 상태와 의존성을 구성합니다.
         */
        UAnimInstance();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UAnimInstance() override;

    public:

        /**
         * @brief 소유 컴포넌트를 연결하고 매핑 표를 비운 뒤 그래프를 만들고 초기화합니다.
         * @details NativeInitializeAnimation을 호출한 뒤 루트 노드부터 Initialize를 전파합니다. 컴포넌트의 메시나 스켈레톤이 바뀔 때와 설정을 편집할 때 다시 호출됩니다.
         * @param[in] Owner 이 인스턴스를 소유하는 컴포넌트입니다.
         */
        void InitializeAnimation(Engine::USkeletalMeshComponent* Owner);

        /**
         * @brief 변수와 그래프를 갱신한 뒤 현재 포즈를 계산합니다.
         * @param[in] DeltaSeconds 지난 프레임 이후 경과 시간입니다.
         */
        void UpdateAnimation(float DeltaSeconds);

        /**
         * @brief 이번 갱신에서 아직 처리하지 않은 노드만 갱신합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @param Context 노드 갱신 시간과 가중치를 제공하는 컨텍스트입니다.
         */
        void UpdateNode(Engine::FAnimNode_Base& Node, const Engine::FAnimationUpdateContext& Context);

        /**
         * @brief 지정 노드를 이번 갱신에서 다시 평가할 수 있도록 기록을 해제합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         */
        void ResetNodeUpdate(Engine::FAnimNode_Base& Node);

        /**
         * @brief 마지막으로 계산한 포즈를 반환합니다.
         * @return 스켈레톤 본 인덱스 순서의 부모 본 기준 변환이며, 스켈레톤이 없으면 비어 있습니다.
         */
        const Core::TArray<Core::FTransform>& GetPose() const;

        /**
         * @brief 루트 모션 추출·전달 정책을 반환합니다.
         * @return 현재 루트 모션 모드입니다.
         */
        Engine::ERootMotionMode GetRootMotionMode() const;

        /**
         * @brief 루트 모션 추출·전달 정책을 변경합니다.
         * @param Mode 설정할 동작 또는 표시 모드입니다.
         */
        void SetRootMotionMode(Engine::ERootMotionMode Mode);

        /**
         * @brief 현재 스켈레톤 매핑과 루트 모션 정책에 따라 시퀀스 구간을 추출합니다.
         * @param Sequence 시간 범위와 프레임률을 제공하는 시퀀스입니다.
         * @param StartTime 구간 시작 시각(초)입니다.
         * @param DeltaTime 루트 모션을 추출할 시간 길이(초)이며 음수는 역방향입니다.
         * @param Looping 끝점을 넘어 반복할지 여부입니다.
         * @return 허용된 구간의 루트 모션이며 추출하지 않으면 항등 변환입니다.
         */
        Core::FTransform ExtractSequenceRootMotion(const Asset::UAnimSequence& Sequence, double StartTime, double DeltaTime, bool Looping);

        /**
         * @brief 최근 갱신에서 계산한 루트 모션 증분을 반환합니다.
         * @return 노드 또는 인스턴스의 이동·회전 증분이며 없으면 항등 변환입니다.
         */
        Core::FTransform GetRootMotionDelta() const;

        /**
         * @brief 소비 전까지 누적한 루트 모션을 반환하고 누적값을 초기화합니다.
         * @return 소비한 루트 모션 변환입니다.
         */
        Core::FTransform ConsumeExtractedRootMotion();

        /**
         * @brief 최근 증분과 소비 전 누적 루트 모션을 항등 변환으로 초기화합니다.
         */
        void ResetRootMotion();

        /**
         * @brief 소유 컴포넌트를 반환합니다.
         * @return 초기화하지 않았으면 nullptr입니다.
         */
        Engine::USkeletalMeshComponent* GetOwningComponent() const;

        /**
         * @brief 그래프의 루트 노드를 반환합니다. 시각화 도구가 그래프를 찾을 때 씁니다.
         * @return 그래프가 없으면 nullptr입니다.
         */
        Engine::FAnimNode_Base* GetRootNode() const;

        /**
         * @brief 최근 포즈 갱신의 노드 방문 기록을 반환합니다.
         * @return 인스턴스 소유 배열의 읽기 전용 참조입니다.
         */
        const Core::TArray<Engine::FAnimNodeVisit>& GetDebugVisits() const;

        /**
         * @brief 부모 방문과 가중치를 연결하여 포즈 노드 방문 기록을 시작합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @param Link 해당 방문을 발생시킨 비소유 포즈 연결입니다.
         * @param Weight 최종 포즈에 기여하는 가중치입니다.
         */
        void BeginDebugVisit(const Engine::FAnimNode_Base& Node, const Engine::FPoseLink* Link, float Weight);

        /**
         * @brief 현재 방문 기록을 종료하고 부모 방문으로 돌아갑니다.
         */
        void EndDebugVisit();

        /**
         * @brief 샘플 기반 노드가 사용한 샘플 인덱스와 가중치를 기록합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @param SampleIndex 샘플 기반 노드에서 사용한 샘플 인덱스입니다.
         * @param Weight 최종 포즈에 기여하는 가중치입니다.
         */
        void RecordDebugSample(const Engine::FAnimNode_Base& Node, Core::Index_t SampleIndex, float Weight);

        /**
         * @brief 소유 컴포넌트의 월드 위치 변화로 구한 속도를 반환합니다.
         * @details 언리얼의 TryGetPawnOwner()->GetVelocity()를 대신합니다. 이 엔진에는 폰 이동 컴포넌트가 없으므로 위치 변화를 씁니다.
         * @return 월드 단위/초 속도이며 첫 갱신에서는 0입니다.
         */
        const Core::FVector& GetOwnerVelocity() const;

        /**
         * @brief 스켈레톤의 레퍼런스 포즈를 반환합니다.
         * @param[out] OutPose 스켈레톤 본 인덱스 순서의 로컬 변환이며, 스켈레톤이 없으면 비웁니다.
         */
        void GetReferencePose(Core::TArray<Core::FTransform>& OutPose) const;

        /**
         * @brief 시퀀스를 지정한 시간으로 샘플링해 트랙이 있는 본의 변환을 덮어씁니다.
         * @param[in] Sequence 샘플링할 시퀀스입니다.
         * @param[in] Time 초 단위 재생 시간입니다.
         * @param[in] Looping 반복 재생 여부입니다.
         * @param[in,out] Pose 스켈레톤 본 인덱스 순서의 로컬 변환입니다.
         */
        void EvaluateSequence(const Asset::UAnimSequence& Sequence, double Time, bool Looping, Core::TArray<Core::FTransform>& Pose);

        /**
         * @brief 리플렉션 프로퍼티가 아닌 실행 중 변수를 시각화 도구에 보이도록 반환합니다.
         * @details 그래프 에셋의 변수처럼 클래스에 고정되지 않은 변수를 위한 확장 지점이며, 기본 구현은 아무것도 추가하지 않습니다.
         * @param[out] OutValues 값을 뒤에 추가합니다.
         */
        virtual void GetDebugVariables(Core::TArray<Engine::FAnimNodeValue>& OutValues);

        /**
         * @brief 이름에 대응하는 Float 그래프 변수 값을 조회합니다.
         * @param Name 조회하거나 설정할 그래프 변수 이름입니다.
         * @return 현재 변수 값이며 유효한 변수가 없으면 0입니다.
         */
        virtual float GetFloatVariable(const Core::FString& Name) const;

        /**
         * @brief 이름에 대응하는 Bool 그래프 변수 값을 조회합니다.
         * @param Name 조회하거나 설정할 그래프 변수 이름입니다.
         * @return 현재 변수 값이며 유효한 변수가 없으면 false입니다.
         */
        virtual bool GetBoolVariable(const Core::FString& Name) const;

        /**
         * @brief 지정한 이름과 Float·Bool 형식의 그래프 변수가 있는지 확인합니다.
         * @param Name 조회하거나 설정할 그래프 변수 이름입니다.
         * @param IsBool Bool 변수를 조회하면 true, Float 변수이면 false입니다.
         * @return 이름과 형식이 모두 일치하면 true입니다.
         */
        virtual bool HasGraphVariable(const Core::FString& Name, bool IsBool) const;

        /**
         * @brief 시각화 도구가 노드를 놓을 위치를 반환합니다.
         * @details 그래프 에셋에서 만든 인스턴스는 에디터에서 배치한 위치를 돌려줍니다. 기본 구현은 위치가 없어 자동 배치합니다.
         * @param[in] Node 노드이며 nullptr이면 Output Pose 노드입니다.
         * @param[out] X 캔버스 X 위치입니다.
         * @param[out] Y 캔버스 Y 위치입니다.
         * @return 위치가 있으면 true입니다.
         */
        virtual bool GetNodeLayout(const Engine::FAnimNode_Base* Node, float& X, float& Y) const;

        /**
         * @brief 편집한 설정을 반영하도록 그래프를 다시 만듭니다.
         * @param[in] Event 변경된 프로퍼티 정보입니다.
         */
        void PostEditChangeProperty(const CoreUObject::FPropertyChangedEvent& Event) override;

    protected:

        /**
         * @brief 초기화할 때 한 번 호출됩니다. 파생 클래스는 여기서 에셋을 로드하고 노드를 연결한 뒤 mRootNode를 지정합니다.
         */
        virtual void NativeInitializeAnimation();

        /**
         * @brief 매 프레임 그래프 갱신 전에 호출되어 변수를 갱신합니다.
         * @param[in] DeltaSeconds 지난 프레임 이후 경과 시간입니다.
         */
        virtual void NativeUpdateAnimation(float DeltaSeconds);

        /**
         * @brief 레퍼런스 포즈로 채운 포즈를 받아 이번 프레임의 포즈로 바꿉니다.
         * @details 기본 구현은 루트 노드가 있으면 Evaluate한 결과를 씁니다.
         * @param[in,out] Pose 스켈레톤 본 인덱스 순서의 로컬 변환입니다.
         */
        virtual void NativeEvaluateAnimation(Core::TArray<Core::FTransform>& Pose);

        /**
         * @brief 포즈 계산에 쓰는 스켈레톤을 반환합니다.
         * @return 소유 컴포넌트의 현재 스켈레톤이며 없으면 nullptr입니다.
         */
        const Asset::USkeleton* GetSkeleton() const;

        /**
         * @brief 소유 컴포넌트의 월드 에셋 관리자로 시퀀스를 로드합니다.
         * @param[in] Path Content 기준 경로이며 비어 있으면 nullptr를 반환합니다.
         * @return 실패하면 경고를 남기고 nullptr를 반환합니다.
         */
        const Asset::UAnimSequence* LoadAnimSequence(const Core::FString& Path) const;

        /**
         * @brief 소유 컴포넌트의 월드 에셋 관리자로 블렌드 스페이스를 로드합니다.
         * @param[in] Path Content 기준 경로이며 비어 있으면 nullptr를 반환합니다.
         * @return 실패하면 경고를 남기고 nullptr를 반환합니다.
         */
        const Asset::UBlendSpace* LoadBlendSpace(const Core::FString& Path) const;

    protected:

        /**
         * @brief 그래프의 시작 노드입니다. 파생 클래스가 소유한 노드를 가리킵니다.
         */
        Engine::FAnimNode_Base* mRootNode{};

    private:

        /**
         * @brief 시퀀스 트랙과 현재 스켈레톤 본의 대응을 조회하거나 생성합니다.
         * @param Sequence 시간 범위와 프레임률을 제공하는 시퀀스입니다.
         * @return 인스턴스가 소유하는 매핑 캐시 참조입니다.
         */
        const FSequenceBoneMapping& GetSequenceBoneMapping(const Asset::UAnimSequence& Sequence);

    private:

        /**
         * @brief 루트 모션 추출·전달 방식입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "RootMotionMode", Getter = GetRootMotionMode, Setter = SetRootMotionMode)
        Engine::ERootMotionMode mRootMotionMode{Engine::ERootMotionMode::NoRootMotionExtraction};

        /**
         * @brief 최근 갱신에서 계산한 루트 모션 증분입니다.
         */
        Core::FTransform mRootMotionDelta{};

        /**
         * @brief 소비 전까지 누적한 루트 모션 변환입니다.
         */
        Core::FTransform mExtractedRootMotion{};

        /**
         * @brief 이 인스턴스를 소유하는 컴포넌트입니다. 컴포넌트가 인스턴스보다 오래 살아 있습니다.
         */
        Engine::USkeletalMeshComponent* mOwner{};

        /**
         * @brief 시퀀스별로 트랙 인덱스를 스켈레톤 본 인덱스로 바꾸는 표이며, 스켈레톤에 없는 트랙은 -1입니다.
         * @details 시퀀스를 처음 평가할 때 만들고, 스켈레톤이 바뀌면 InitializeAnimation이 비웁니다.
         */
        Core::TMap<const Asset::UAnimSequence*, FSequenceBoneMapping> mTrackToBone{};

        /**
         * @brief 샘플링 결과를 담는 트랙 순서의 임시 포즈입니다.
         */
        Core::TArray<Core::FTransform> mTrackPose{};

        /**
         * @brief 마지막으로 계산한 본 인덱스 순서의 포즈입니다.
         */
        Core::TArray<Core::FTransform> mPose{};

        /**
         * @brief 소유 컴포넌트의 속도입니다.
         */
        Core::FVector mOwnerVelocity{};

        /**
         * @brief 이전 갱신의 소유 컴포넌트 월드 위치입니다.
         */
        Core::FVector mLastOwnerLocation{};

        /**
         * @brief mLastOwnerLocation이 유효한지 여부입니다.
         */
        bool mHasLastOwnerLocation{};

        /**
         * @brief 이번 갱신에서 방문한 포즈 노드와 가중치 기록입니다.
         */
        Core::TArray<Engine::FAnimNodeVisit> mDebugVisits{};

        /**
         * @brief 현재 기록 중인 방문 인덱스이며 비활성 시 -1입니다.
         */
        Core::Index_t mCurrentDebugVisit{-1};

        /**
         * @brief 동일 갱신에서 중복 평가하지 않도록 기록하는 포즈 노드 집합입니다.
         */
        Core::TMap<Engine::FAnimNode_Base*, bool> mUpdatedNodes{};
    };

}
