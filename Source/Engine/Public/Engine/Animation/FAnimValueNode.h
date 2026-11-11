#pragma once

#include "Core/Types.h"

namespace Engine {

    struct FAnimationUpdateContext;

    enum class EAnimValueType : Core::Uint8 {
        Float,
        Bool
    };

    struct FAnimValueNode_Base {
    public:
        explicit FAnimValueNode_Base(Engine::EAnimValueType ValueType);
        virtual ~FAnimValueNode_Base() = default;

    public:
        Engine::EAnimValueType GetValueType() const;

        virtual const Core::FString& GetNodeType() const = 0;
        virtual const Core::FString& GetDisplayName() const;

    private:
        const Engine::EAnimValueType mValueType{};
    };

    struct FAnimValueNode_Float : public Engine::FAnimValueNode_Base {
    public:
        FAnimValueNode_Float();

    public:
        virtual float Evaluate(const Engine::FAnimationUpdateContext& Context) const = 0;
    };

    struct FAnimValueNode_GetFloat : public Engine::FAnimValueNode_Float {
    public:
        float Evaluate(const Engine::FAnimationUpdateContext& Context) const override;
        const Core::FString& GetNodeType() const override;
        const Core::FString& GetDisplayName() const override;

    public:
        Core::FString mVariableName{};
    };

    struct FAnimValueLink_Base {
    public:
        explicit FAnimValueLink_Base(Engine::EAnimValueType ValueType);
        virtual ~FAnimValueLink_Base() = default;

    public:
        Engine::EAnimValueType GetValueType() const;

        virtual Engine::FAnimValueNode_Base* GetLinkedNode() const = 0;
        virtual bool SetLinkedNode(Engine::FAnimValueNode_Base* Node) = 0;

    private:
        const Engine::EAnimValueType mValueType{};
    };

    struct FAnimFloatLink : public Engine::FAnimValueLink_Base {
    public:
        FAnimFloatLink();

    public:
        Engine::FAnimValueNode_Float* GetLinkedNode() const override;
        bool SetLinkedNode(Engine::FAnimValueNode_Base* Node) override;

        float Evaluate(const Engine::FAnimationUpdateContext& Context, float DefaultValue) const;

    private:
        Engine::FAnimValueNode_Float* mLinkedNode{};
    };

}
