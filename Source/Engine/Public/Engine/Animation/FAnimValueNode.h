/**
 * @file FAnimValueNode.h
 * @brief 애니메이션 값 노드와 자료형을 검증하는 입력 연결을 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"

namespace Engine {

    struct FAnimationUpdateContext;

    /**
     * @brief 애니메이션 값 노드와 입력 핀의 자료형입니다.
     * @ingroup Engine
     */
    enum class EAnimValueType : Core::Uint8 {

        /**
         * @brief Float 값을 연결합니다.
         */
        Float,

        /**
         * @brief Bool 값을 연결합니다.
         */
        Bool
    };

    /**
     * @brief 애니메이션 그래프의 값 형식과 표시 이름을 제공하는 값 노드 기반형입니다.
     * @ingroup Engine
     */
    struct FAnimValueNode_Base {
    public:

        /**
         * @brief 노드가 출력할 값 형식을 고정합니다.
         * @param ValueType 노드 또는 입력이 지원할 값 형식입니다.
         */
        explicit FAnimValueNode_Base(Engine::EAnimValueType ValueType);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~FAnimValueNode_Base() = default;

    public:

        /**
         * @brief 노드 또는 입력이 지원하는 값 형식을 반환합니다.
         * @return Float 또는 Bool 형식입니다.
         */
        Engine::EAnimValueType GetValueType() const;

        /**
         * @brief 값 노드의 등록 종류 이름을 반환합니다.
         * @return 정적 종류 이름의 읽기 전용 참조입니다.
         */
        virtual const Core::FString& GetNodeType() const = 0;

        /**
         * @brief 값 노드의 디버그 표시 이름을 반환합니다.
         * @return 노드 또는 정적 저장소가 보관하는 문자열의 읽기 전용 참조입니다.
         */
        virtual const Core::FString& GetDisplayName() const;

    private:

        /**
         * @brief 노드 또는 입력이 지원하는 불변 값 형식입니다.
         */
        const Engine::EAnimValueType mValueType{};
    };

    /**
     * @brief 갱신 컨텍스트에서 Float 값을 평가하는 노드 기반형입니다.
     * @ingroup Engine
     */
    struct FAnimValueNode_Float : public Engine::FAnimValueNode_Base {
    public:

        /**
         * @brief Float 출력을 갖는 값 노드를 구성합니다.
         */
        FAnimValueNode_Float();

    public:

        /**
         * @brief 현재 애니메이션 갱신 컨텍스트에서 Float 값을 평가합니다.
         * @param Context 값 노드 평가에 사용할 애니메이션 갱신 컨텍스트입니다.
         * @return 노드가 공급하는 Float 값입니다.
         */
        virtual float Evaluate(const Engine::FAnimationUpdateContext& Context) const = 0;
    };

    /**
     * @brief 애니메이션 인스턴스에서 지정한 Float 그래프 변수를 읽는 값 노드입니다.
     * @ingroup Engine
     */
    struct FAnimValueNode_GetFloat : public Engine::FAnimValueNode_Float {
    public:

        /**
         * @brief 현재 인스턴스에서 이름에 대응하는 Float 변수를 읽습니다.
         * @param Context 그래프 변수를 조회할 애니메이션 갱신 컨텍스트입니다.
         * @return 변수 값이며 유효한 인스턴스·변수가 없으면 0입니다.
         */
        float Evaluate(const Engine::FAnimationUpdateContext& Context) const override;

        /**
         * @brief 값 노드의 등록 종류 이름을 반환합니다.
         * @return 정적 종류 이름의 읽기 전용 참조입니다.
         */
        const Core::FString& GetNodeType() const override;

        /**
         * @brief 값 노드의 디버그 표시 이름을 반환합니다.
         * @return 노드 또는 정적 저장소가 보관하는 문자열의 읽기 전용 참조입니다.
         */
        const Core::FString& GetDisplayName() const override;

    public:

        /**
         * @brief 조회할 Float 그래프 변수 이름입니다.
         */
        Core::FString mVariableName{};
    };

    /**
     * @brief 기대하는 값 형식을 보관하고 값 노드 연결을 검증하는 입력 기반형입니다.
     * @ingroup Engine
     */
    struct FAnimValueLink_Base {
    public:

        /**
         * @brief 입력이 허용할 값 형식을 고정합니다.
         * @param ValueType 노드 또는 입력이 지원할 값 형식입니다.
         */
        explicit FAnimValueLink_Base(Engine::EAnimValueType ValueType);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~FAnimValueLink_Base() = default;

    public:

        /**
         * @brief 노드 또는 입력이 지원하는 값 형식을 반환합니다.
         * @return Float 또는 Bool 형식입니다.
         */
        Engine::EAnimValueType GetValueType() const;

        /**
         * @brief 입력에 연결된 값 노드를 조회합니다.
         * @return 외부 소유의 비소유 포인터이며 미연결 시 nullptr입니다.
         */
        virtual Engine::FAnimValueNode_Base* GetLinkedNode() const = 0;

        /**
         * @brief 값 형식이 맞는 노드를 연결하거나 nullptr로 연결을 해제합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @return 연결할 값 형식이 호환되면 true입니다.
         */
        virtual bool SetLinkedNode(Engine::FAnimValueNode_Base* Node) = 0;

    private:

        /**
         * @brief 노드 또는 입력이 지원하는 불변 값 형식입니다.
         */
        const Engine::EAnimValueType mValueType{};
    };

    /**
     * @brief Float 값 노드를 형식에 맞게 연결하고 미연결 시 기본값을 반환하는 입력입니다.
     * @ingroup Engine
     */
    struct FAnimFloatLink : public Engine::FAnimValueLink_Base {
    public:

        /**
         * @brief Float 값 노드만 연결하는 빈 입력을 구성합니다.
         */
        FAnimFloatLink();

    public:

        /**
         * @brief 입력에 연결된 값 노드를 조회합니다.
         * @return 외부 소유의 비소유 포인터이며 미연결 시 nullptr입니다.
         */
        Engine::FAnimValueNode_Float* GetLinkedNode() const override;

        /**
         * @brief 값 형식이 맞는 노드를 연결하거나 nullptr로 연결을 해제합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @return 연결할 값 형식이 호환되면 true입니다.
         */
        bool SetLinkedNode(Engine::FAnimValueNode_Base* Node) override;

        /**
         * @brief 연결된 Float 노드를 평가하고 미연결이면 지정 기본값을 사용합니다.
         * @param Context 값 노드 평가에 사용할 애니메이션 갱신 컨텍스트입니다.
         * @param DefaultValue 값 노드가 연결되지 않았을 때 사용할 기본값입니다.
         * @return 노드 평가 값 또는 DefaultValue입니다.
         */
        float Evaluate(const Engine::FAnimationUpdateContext& Context, float DefaultValue) const;

    private:

        /**
         * @brief 외부에서 소유하는 연결된 Float 값 노드입니다.
         */
        Engine::FAnimValueNode_Float* mLinkedNode{};
    };

}
