/**
 * @file UScriptComponent.cpp
 * @brief 컴포넌트 수명·겹침 이벤트를 C# 스크립트에 전달합니다.
 * @ingroup Script
 */

#include "Script/UScriptComponent.h"
#include "Script/FScriptRuntime.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/AActor.h"
#include "Engine/Component/UShapeComponent.h"

#include <algorithm>
#include <vector>

namespace Script {

    /**
     * @brief 클래스가 지정되지 않은 컴포넌트를 구성합니다.
     */
    Script::UScriptComponent::UScriptComponent() = default;

    /**
     * @brief 남아 있는 C# 인스턴스를 해제합니다.
     */
    Script::UScriptComponent::~UScriptComponent() {
        UnbindOverlapEvents();

        if (mInstance != 0) {
            Script::FScriptRuntime::Get()->DestroyInstance(mInstance);
        }
    }

    /**
     * @brief C# 인스턴스를 만들고 BeginPlay를 전달합니다.
     */
    void Script::UScriptComponent::BeginPlay() {
        Engine::UActorComponent::BeginPlay();

        if (mScriptClass.empty() || GetOwner() == nullptr) {
            return;
        }

        Script::FScriptRuntime* Runtime{Script::FScriptRuntime::Get()};

        mInstance = Runtime->CreateInstance(mScriptClass, GetOwner());

        if (mInstance != 0) {
            for (const Script::FScriptField& Field : mFields) {
                Runtime->SetInstanceField(mInstance, Field);
            }

            BindOverlapEvents();
            Runtime->BeginPlayInstance(mInstance);
        }
    }

    /**
     * @brief C# 인스턴스에 Tick을 전달합니다.
     */
    void Script::UScriptComponent::Tick(float DeltaTime) {
        Engine::UActorComponent::Tick(DeltaTime);

        if (mInstance != 0) {
            Script::FScriptRuntime::Get()->TickInstance(mInstance, DeltaTime);
        }
    }

    /**
     * @brief C# 인스턴스에 EndPlay를 전달하고 해제합니다.
     */
    void Script::UScriptComponent::EndPlay() {
        UnbindOverlapEvents();

        if (mInstance != 0) {
            Script::FScriptRuntime* Runtime{Script::FScriptRuntime::Get()};
            Runtime->EndPlayInstance(mInstance);
            Runtime->DestroyInstance(mInstance);

            mInstance = 0;
        }

        Engine::UActorComponent::EndPlay();
    }

    /**
     * @brief 소유 액터의 충돌 도형 이벤트를 C# 인스턴스로 연결합니다.
     */
    void Script::UScriptComponent::BindOverlapEvents() {
        const auto Notify{[this](bool Begin, const Engine::AActor* Other) {
            if (mInstance != 0) {
                Script::FScriptRuntime::Get()->OverlapInstance(mInstance, Begin, Other);
            }
        }};

        for (const auto& Component : GetOwner()->GetComponents()) {
            if (auto* Shape{CoreUObject::Cast<Engine::UShapeComponent>(Component.get())}) {
                const CoreUObject::FDelegateHandle Begin{Shape->GetOnComponentBeginOverlap().Add([Notify](Engine::UPrimitiveComponent*, Engine::AActor* Other, Engine::UPrimitiveComponent*) {
                    Notify(true, Other);
                })};
                const CoreUObject::FDelegateHandle End{Shape->GetOnComponentEndOverlap().Add([Notify](Engine::UPrimitiveComponent*, Engine::AActor* Other, Engine::UPrimitiveComponent*) {
                    Notify(false, Other);
                })};
                mOverlapBindings.push_back({Shape, Begin, End});
            }
        }
    }

    /**
     * @brief 충돌 도형에 등록한 이벤트를 모두 해제합니다.
     */
    void Script::UScriptComponent::UnbindOverlapEvents() {
        for (const FOverlapBinding& Binding : mOverlapBindings) {
            if (auto* Shape{Binding.mShape.Get()}) {
                Shape->GetOnComponentBeginOverlap().Remove(Binding.mBegin);
                Shape->GetOnComponentEndOverlap().Remove(Binding.mEnd);
            }
        }

        mOverlapBindings.clear();
    }

    /**
     * @brief 스크립트 클래스가 바뀌면 이전 클래스의 필드 값을 버립니다.
     */
    void Script::UScriptComponent::PostEditChangeProperty(const CoreUObject::FPropertyChangedEvent& Event) {
        Engine::UActorComponent::PostEditChangeProperty(Event);

        if (Event.mProperty != nullptr && Event.mProperty->GetName() == "ScriptClass") {
            mFields.clear();
        }
    }

    /**
     * @brief 네임스페이스를 포함한 C# 클래스 이름을 반환합니다.
     */
    const Core::FString& Script::UScriptComponent::GetScriptClass() const {
        return mScriptClass;
    }

    /**
     * @brief 기본값과 다르게 저장된 필드 목록을 반환합니다.
     */
    const Core::TArray<Script::FScriptField>& Script::UScriptComponent::GetFields() const {
        return mFields;
    }

    /**
     * @brief 필드 값을 저장합니다. 같은 이름이 있으면 교체합니다.
     */
    void Script::UScriptComponent::SetField(const Script::FScriptField& Field) {
        const auto Position{std::find_if(mFields.begin(), mFields.end(), [&Field](const Script::FScriptField& Candidate) {
            return Candidate.mName == Field.mName;
        })};

        if (Position != mFields.end()) {
            *Position = Field;
        } else {
            mFields.push_back(Field);
        }
    }

    /**
     * @brief 저장된 필드를 지워 스크립트의 기본값으로 되돌립니다.
     */
    void Script::UScriptComponent::RemoveField(const Core::FString& Name) {
        std::erase_if(mFields, [&Name](const Script::FScriptField& Field) {
            return Field.mName == Name;
        });
    }

}
