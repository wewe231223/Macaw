/**
 * @file FAnimNode_Base.h
 * @brief 애니메이션 그래프 노드의 기반과 노드 사이에 오가는 갱신·포즈 컨텍스트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FTransform.h"

namespace Engine {

    class UAnimInstance;
    struct FAnimNode_Base;

    /**
     * @brief Update 단계에서 노드에 전달하는 경과 시간과 누적 가중치입니다.
     * @details 언리얼 FAnimationUpdateContext에 대응합니다. 블렌드 노드는 FractionalWeight로 입력마다 가중치를 나눠 전달하므로,
     * 각 노드는 자신이 최종 포즈에 얼마나 기여하는지 알 수 있습니다. 시각화 도구는 이 값으로 활성 노드를 강조합니다.
     */
    struct FAnimationUpdateContext {
    public:
        /**
         * @brief 그래프 루트에 전달할 컨텍스트를 만듭니다.
         * @param[in] AnimInstance 그래프를 소유한 인스턴스입니다.
         * @param[in] DeltaTime 초 단위 경과 시간입니다.
         * @param[in] UpdateCounter 인스턴스가 갱신할 때마다 1씩 늘리는 번호입니다.
         */
        FAnimationUpdateContext(Engine::UAnimInstance& AnimInstance, float DeltaTime, Core::Uint64 UpdateCounter);

    public:
        /**
         * @brief 가중치에 배율을 곱한 컨텍스트를 만듭니다.
         * @param[in] Multiplier 0과 1 사이의 배율입니다.
         * @return 가중치만 다른 복사본입니다.
         */
        Engine::FAnimationUpdateContext FractionalWeight(float Multiplier) const;

        /**
         * @brief 그래프를 소유한 인스턴스를 반환합니다.
         * @return 인스턴스의 참조입니다.
         */
        Engine::UAnimInstance& GetAnimInstance() const;

        /**
         * @brief 초 단위 경과 시간을 반환합니다.
         * @return 이번 갱신의 경과 시간입니다.
         */
        float GetDeltaTime() const;

        /**
         * @brief 이 노드가 최종 포즈에 기여하는 가중치를 반환합니다.
         * @return 루트는 1이며 블렌드 노드를 지날 때마다 나뉩니다.
         */
        float GetFinalBlendWeight() const;

        /**
         * @brief 이번 갱신의 번호를 반환합니다.
         * @return 인스턴스의 갱신 번호입니다.
         */
        Core::Uint64 GetUpdateCounter() const;

    private:
        /**
         * @brief 그래프를 소유한 인스턴스입니다.
         */
        Engine::UAnimInstance* mAnimInstance{};

        /**
         * @brief 초 단위 경과 시간입니다.
         */
        float mDeltaTime{};

        /**
         * @brief 누적 가중치입니다.
         */
        float mWeight{1.0f};

        /**
         * @brief 인스턴스의 갱신 번호입니다.
         */
        Core::Uint64 mUpdateCounter{};
    };

    /**
     * @brief Evaluate 단계에서 노드가 채우는 포즈입니다.
     * @details 언리얼 FPoseContext에 대응하며, 포즈는 스켈레톤 본 인덱스 순서의 부모 본 기준 변환입니다.
     * 만들 때 스켈레톤의 레퍼런스 포즈로 채우므로, 트랙이 없는 본은 레퍼런스 포즈를 유지합니다. 커브는 아직 다루지 않습니다.
     */
    struct FPoseContext {
    public:
        /**
         * @brief 레퍼런스 포즈로 채운 컨텍스트를 만듭니다.
         * @param[in] AnimInstance 포즈를 계산하는 인스턴스입니다.
         */
        explicit FPoseContext(Engine::UAnimInstance& AnimInstance);

    public:
        /**
         * @brief 포즈를 계산하는 인스턴스를 반환합니다.
         * @return 인스턴스의 참조입니다.
         */
        Engine::UAnimInstance& GetAnimInstance() const;

        /**
         * @brief 포즈를 스켈레톤의 레퍼런스 포즈로 되돌립니다.
         */
        void ResetToReferencePose();

    public:
        /**
         * @brief 스켈레톤 본 인덱스 순서의 로컬 변환입니다. 스켈레톤이 없으면 비어 있습니다.
         */
        Core::TArray<Core::FTransform> mPose{};

    private:
        /**
         * @brief 포즈를 계산하는 인스턴스입니다.
         */
        Engine::UAnimInstance* mAnimInstance{};
    };

    /**
     * @brief 다른 노드의 출력 포즈를 입력으로 받는 연결입니다.
     * @details 언리얼 FPoseLink에 대응합니다. 연결된 노드가 없으면 Evaluate는 받은 포즈를 그대로 둡니다.
     */
    struct FPoseLink {
    public:
        /**
         * @brief 연결된 노드를 초기화합니다.
         * @param[in] AnimInstance 그래프를 소유한 인스턴스입니다.
         */
        void Initialize(Engine::UAnimInstance& AnimInstance);

        /**
         * @brief 연결된 노드에 갱신 기록을 남기고 Update를 전달합니다.
         * @param[in] Context 갱신 컨텍스트입니다.
         */
        void Update(const Engine::FAnimationUpdateContext& Context);

        /**
         * @brief 연결된 노드의 포즈를 계산합니다.
         * @param[in,out] Output 레퍼런스 포즈로 채운 컨텍스트이며 노드가 결과로 덮어씁니다.
         */
        void Evaluate(Engine::FPoseContext& Output);

        float GetDebugWeight(Core::Uint64 UpdateCounter) const;

    public:
        /**
         * @brief 연결된 노드입니다. 노드는 인스턴스가 소유합니다.
         */
        Engine::FAnimNode_Base* mLinkedNode{};

    private:
        Core::Uint64 mLastUpdateCounter{};
        float mLastWeight{};
    };

    /**
     * @brief 시각화 도구에 보이는 노드의 포즈 입력 핀입니다.
     */
    struct FAnimNodePoseInput {
    public:
        /**
         * @brief 핀 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 핀에 해당하는 연결입니다. 노드가 소유합니다.
         */
        Engine::FPoseLink* mLink{};

        /**
         * @brief 이번 갱신에서 이 입력이 받은 가중치입니다.
         */
        float mWeight{};
    };

    /**
     * @brief 시각화 도구에 보이는 노드 값의 종류입니다.
     */
    enum class EAnimNodeValueType : Core::Uint8 {
        /**
         * @brief 편집할 수 있는 실수입니다.
         */
        Float,
        /**
         * @brief 편집할 수 있는 참/거짓입니다.
         */
        Bool,
        /**
         * @brief 읽기 전용 문자열입니다.
         */
        Text
    };

    /**
     * @brief 시각화 도구에 보이는 노드의 입력 값이나 상태입니다.
     */
    struct FAnimNodeValue {
    public:
        /**
         * @brief 값 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 값의 종류입니다.
         */
        Engine::EAnimNodeValueType mType{Engine::EAnimNodeValueType::Text};

        /**
         * @brief Float 값의 주소입니다. 노드가 소유합니다.
         */
        float* mFloat{};

        /**
         * @brief Bool 값의 주소입니다. 노드가 소유합니다.
         */
        bool* mBool{};

        /**
         * @brief Text 값입니다.
         */
        Core::FString mText{};

        /**
         * @brief Float 값을 편집할 때의 최솟값입니다. 최댓값보다 작지 않으면 제한하지 않습니다.
         */
        float mMin{};

        /**
         * @brief Float 값을 편집할 때의 최댓값입니다.
         */
        float mMax{};

        /**
         * @brief 인스턴스 변수에 묶여 매 갱신 덮어써지는 값이면 그 변수 이름입니다. 비어 있지 않으면 편집할 수 없습니다.
         */
        Core::FString mBinding{};
        const char* mBindingParameter{};
    };

    /**
     * @brief 애니메이션 그래프 노드의 기반입니다.
     * @details 언리얼 FAnimNode_Base에 대응합니다. 인스턴스가 매 프레임 루트에서 Update를 호출해 재생 시간과 가중치를 전파한 뒤,
     * Evaluate를 호출해 포즈를 계산합니다. 노드는 리플렉션 대상이 아닌 일반 구조체이며 인스턴스가 멤버로 소유합니다.
     *
     * 시각화 도구는 루트에서 GetPoseInputs를 따라 그래프를 찾고, GetDebugValues로 노드 값을 표시·편집합니다.
     */
    struct FAnimNode_Base {
    public:
        FAnimNode_Base() = default;

        virtual ~FAnimNode_Base() = default;

        /**
         * @brief 노드는 그래프 안의 위치로 식별되므로 복사를 금지합니다.
         */
        FAnimNode_Base(const Engine::FAnimNode_Base&) = delete;

        /**
         * @brief 노드는 그래프 안의 위치로 식별되므로 대입을 금지합니다.
         * @return 자기 자신의 참조입니다.
         */
        Engine::FAnimNode_Base& operator=(const Engine::FAnimNode_Base&) = delete;

    public:
        /**
         * @brief 인스턴스를 초기화할 때 한 번 호출되어 재생 상태를 처음으로 되돌리고 입력 노드에 전파합니다.
         * @param[in] AnimInstance 그래프를 소유한 인스턴스입니다.
         */
        virtual void Initialize(Engine::UAnimInstance& AnimInstance);

        /**
         * @brief 재생 시간과 블렌드 가중치를 갱신하고 입력 노드에 전파합니다.
         * @param[in] Context 갱신 컨텍스트입니다.
         */
        virtual void Update(const Engine::FAnimationUpdateContext& Context);

        /**
         * @brief 이 노드의 출력 포즈를 계산합니다.
         * @param[in,out] Output 레퍼런스 포즈로 채운 컨텍스트이며 결과로 덮어씁니다.
         */
        virtual void Evaluate(Engine::FPoseContext& Output);

        /**
         * @brief 노드 종류의 표시 이름을 반환합니다.
         * @return 예를 들어 "Sequence Player"입니다.
         */
        virtual const char* GetNodeType() const = 0;

        /**
         * @brief 포즈 입력 핀을 반환합니다.
         * @param[out] OutInputs 입력 핀을 뒤에 추가합니다.
         */
        virtual void GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs);

        /**
         * @brief 시각화 도구에 보일 입력 값과 상태를 반환합니다.
         * @param[out] OutValues 값을 뒤에 추가합니다.
         */
        virtual void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues);

        /**
         * @brief 마지막 Update에서 받은 가중치를 반환합니다.
         * @param[in] UpdateCounter 인스턴스의 현재 갱신 번호입니다.
         * @return 이번 갱신에서 Update를 받지 않았으면 0입니다.
         */
        float GetDebugWeight(Core::Uint64 UpdateCounter) const;

        /**
         * @brief 이번 갱신에서 받은 가중치를 기록합니다. FPoseLink와 인스턴스가 Update 직전에 호출합니다.
         * @param[in] Context 갱신 컨텍스트입니다.
         */
        void RecordUpdate(const Engine::FAnimationUpdateContext& Context);

    public:
        /**
         * @brief 시각화 도구에 보이는 노드 이름입니다. 비어 있으면 노드 종류를 표시합니다.
         */
        Core::FString mName{};

        /**
         * @brief 그래프 에셋에서 만든 노드이면 에셋 안의 노드 식별자이며, C++로 만든 노드는 -1입니다.
         * @details 시각화 도구는 플레이할 때마다 새로 만들어지는 노드를 이 값(또는 인스턴스 안의 멤버 위치)으로 식별해, 사용자가 옮긴 위치를 유지합니다.
         */
        Core::Int32 mGraphNodeId{-1};

    private:
        /**
         * @brief 마지막 Update에서 받은 가중치입니다.
         */
        float mLastWeight{};

        /**
         * @brief 마지막 Update를 받은 갱신 번호입니다.
         */
        Core::Uint64 mLastUpdateCounter{};
    };

}
