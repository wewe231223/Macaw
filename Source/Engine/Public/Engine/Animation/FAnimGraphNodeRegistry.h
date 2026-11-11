/**
 * @file FAnimGraphNodeRegistry.h
 * @brief 애니메이션 그래프 에셋에 배치할 수 있는 노드 종류와 그 핀·파라미터, 노드를 만드는 방법을 정의합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"

#include <functional>
#include <memory>

namespace Asset {
    class UAnimSequence;
    class UBlendSpace;
    struct FAnimGraphNode;
}

namespace Engine {

    struct FAnimNode_Base;

    /**
     * @brief 노드 파라미터 값의 종류입니다.
     */
    enum class EAnimGraphParameterType : Core::Uint8 {
        /**
         * @brief 실수입니다.
         */
        Float,
        /**
         * @brief 참/거짓입니다. "true" 또는 "false"로 저장합니다.
         */
        Bool,
        /**
         * @brief 애니메이션 시퀀스의 Content 기준 경로입니다.
         */
        AnimSequence,
        /**
         * @brief 블렌드 스페이스의 Content 기준 경로입니다.
         */
        BlendSpace
    };

    /**
     * @brief 노드 종류가 받는 파라미터 하나의 설명입니다.
     */
    struct FAnimGraphParameterInfo {
    public:
        /**
         * @brief 파라미터 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 값의 종류입니다.
         */
        Engine::EAnimGraphParameterType mType{Engine::EAnimGraphParameterType::Float};

        /**
         * @brief 저장된 값이 없을 때 쓰는 값입니다.
         */
        Core::FString mDefault{};

        /**
         * @brief 실수 파라미터를 그래프 변수에 묶을 수 있는지 여부입니다. 묶은 변수 이름은 "이름.Binding" 파라미터로 저장합니다.
         */
        bool mBindable{};

        /**
         * @brief 편집 범위의 최솟값입니다. 최댓값보다 작지 않으면 제한하지 않습니다.
         */
        float mMin{};

        /**
         * @brief 편집 범위의 최댓값입니다.
         */
        float mMax{};
    };

    /**
     * @brief 노드 하나를 만들 때 필요한 데이터와 에셋 조회 함수입니다.
     */
    struct FAnimGraphNodeBuildContext {
    public:
        /**
         * @brief 실수 파라미터를 읽습니다. 저장된 값이 없거나 해석할 수 없으면 기본값을 씁니다.
         * @param[in] Name 파라미터 이름입니다.
         * @return 실수 값입니다.
         */
        float GetFloat(const Core::FString& Name) const;

        /**
         * @brief 참/거짓 파라미터를 읽습니다.
         * @param[in] Name 파라미터 이름입니다.
         * @return 저장된 값이 "true"이면 true입니다.
         */
        bool GetBool(const Core::FString& Name) const;

        /**
         * @brief 문자열 파라미터를 읽습니다.
         * @param[in] Name 파라미터 이름입니다.
         * @return 저장된 값이며 없으면 기본값입니다.
         */
        Core::FString GetString(const Core::FString& Name) const;

    public:
        /**
         * @brief 만들 노드의 저장 데이터입니다.
         */
        const Asset::FAnimGraphNode* mNode{};

        /**
         * @brief 노드 종류의 기본값을 찾을 파라미터 설명입니다.
         */
        const Core::TArray<Engine::FAnimGraphParameterInfo>* mParameters{};

        /**
         * @brief 시퀀스 경로를 로드합니다.
         */
        std::function<const Asset::UAnimSequence*(const Core::FString&)> mLoadSequence{};

        /**
         * @brief 블렌드 스페이스 경로를 로드합니다.
         */
        std::function<const Asset::UBlendSpace*(const Core::FString&)> mLoadBlendSpace{};
    };

    /**
     * @brief 그래프 에셋에 배치할 수 있는 노드 종류 하나입니다.
     */
    struct FAnimGraphNodeType {
    public:
        /**
         * @brief 저장되는 종류 이름입니다. 예를 들어 "SequencePlayer"입니다.
         */
        Core::FString mType{};

        /**
         * @brief 에디터에 보이는 이름입니다.
         */
        Core::FString mTitle{};

        /**
         * @brief 포즈 입력 핀 이름입니다. 만든 노드의 GetPoseInputs 순서와 같습니다.
         */
        Core::TArray<Core::FString> mPoseInputs{};

        /**
         * @brief 받는 파라미터입니다.
         */
        Core::TArray<Engine::FAnimGraphParameterInfo> mParameters{};

        /**
         * @brief 저장 데이터로 실행할 노드를 만듭니다.
         */
        std::function<std::unique_ptr<Engine::FAnimNode_Base>(const Engine::FAnimGraphNodeBuildContext&)> mCreate{};
    };

    /**
     * @brief 그래프 에셋에서 쓸 수 있는 노드 종류 목록입니다.
     * @details 새 노드를 그래프 에디터에서 쓰려면 FAnimNode_Base를 상속한 노드를 만들고 이 목록에 종류를 추가합니다.
     */
    class FAnimGraphNodeRegistry {
    public:
        /**
         * @brief 모든 노드 종류를 반환합니다.
         * @return 등록 순서의 목록입니다.
         */
        static const Core::TArray<Engine::FAnimGraphNodeType>& GetNodeTypes();

        /**
         * @brief 종류 이름으로 노드 종류를 찾습니다.
         * @param[in] Type 저장된 종류 이름입니다.
         * @return 없으면 nullptr입니다.
         */
        static const Engine::FAnimGraphNodeType* Find(const Core::FString& Type);
    };

}
