/**
 * @file FAnimNode_Base.h
 * @brief 애니메이션 그래프 노드의 기반과 노드 사이에 오가는 갱신·포즈 컨텍스트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Engine/Animation/FAnimValueNode.h"
#include "Core/Math/FTransform.h"

#include <variant>

namespace Engine {

    class UAnimInstance;
    struct FAnimNode_Base;

    /**
     * @brief Update 단계에서 노드에 전달하는 소유 인스턴스와 경과 시간입니다.
     */
    struct FAnimationUpdateContext {
    public:
        /**
         * @brief 그래프 루트에 전달할 컨텍스트를 만듭니다.
         * @param[in] AnimInstance 그래프를 소유한 인스턴스입니다.
         * @param[in] DeltaTime 초 단위 경과 시간입니다.
         */
        FAnimationUpdateContext(Engine::UAnimInstance& AnimInstance, float DeltaTime);

    public:
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

    private:
        /**
         * @brief 그래프를 소유한 인스턴스입니다.
         */
        Engine::UAnimInstance* mAnimInstance{};

        /**
         * @brief 초 단위 경과 시간입니다.
         */
        float mDeltaTime{};
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
         * @brief 연결된 노드에 Update를 전달합니다.
         * @param[in] Context 갱신 컨텍스트입니다.
         */
        void Update(const Engine::FAnimationUpdateContext& Context);

        /**
         * @brief 연결된 노드의 포즈를 계산합니다.
         * @param[in,out] Output 레퍼런스 포즈로 채운 컨텍스트이며 노드가 결과로 덮어씁니다.
         */
        void Evaluate(Engine::FPoseContext& Output, float Weight = 1.0f);

    public:
        /**
         * @brief 연결된 노드입니다. 노드는 인스턴스가 소유합니다.
         */
        Engine::FAnimNode_Base* mLinkedNode{};
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
    };

    struct FAnimNodeValueInput {
    public:
        Core::FString mName{};
        Engine::FAnimValueLink_Base* mLink{};
    };

    struct FAnimNodeFloatValue {
    public:
        float* mValue{};

        /**
         * @brief Float 값을 편집할 때의 최솟값입니다. 최댓값보다 작지 않으면 제한하지 않습니다.
         */
        float mMin{};

        /**
         * @brief Float 값을 편집할 때의 최댓값입니다.
         */
        float mMax{};
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

        std::variant<Core::FString, Engine::FAnimNodeFloatValue, bool*> mValue{};
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
        virtual const Core::FString& GetNodeType() const = 0;

        /**
         * @brief 포즈 입력 핀을 반환합니다.
         * @param[out] OutInputs 입력 핀을 뒤에 추가합니다.
         */
        virtual void GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs);
        virtual void GetValueInputs(Core::TArray<Engine::FAnimNodeValueInput>& OutInputs);

        /**
         * @brief 시각화 도구에 보일 입력 값과 상태를 반환합니다.
         * @param[out] OutValues 값을 뒤에 추가합니다.
         */
        virtual void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues);

    public:
        /**
         * @brief 시각화 도구에 보이는 노드 이름입니다. 비어 있으면 노드 종류를 표시합니다.
         */
        Core::FString mName{};
    };

}
