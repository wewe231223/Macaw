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

    Script::UScriptComponent::UScriptComponent() = default;

    Script::UScriptComponent::~UScriptComponent() {
        UnbindOverlapEvents();

        if (mInstance != 0) {
            Script::FScriptRuntime::Get()->DestroyInstance(mInstance);
        }
    }

    void Script::UScriptComponent::BeginPlay() {
        Engine::UActorComponent::BeginPlay();

        if (mScriptClass.empty() || GetOwner() == nullptr) {
            return;
        }

        Script::FScriptRuntime* Runtime{Script::FScriptRuntime::Get()};
        mInstance = Runtime->CreateInstance(mScriptClass, GetOwner()->GetHandle());

        if (mInstance != 0) {
            for (const Script::FScriptField& Field : mFields) {
                Runtime->SetInstanceField(mInstance, Field);
            }

            BindOverlapEvents();
            Runtime->BeginPlayInstance(mInstance);
        }
    }

    void Script::UScriptComponent::Tick(float DeltaTime) {
        Engine::UActorComponent::Tick(DeltaTime);

        if (mInstance != 0) {
            Script::FScriptRuntime::Get()->TickInstance(mInstance, DeltaTime);
        }
    }

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

    void Script::UScriptComponent::BindOverlapEvents() {
        const auto Notify{[this](bool Begin, const Engine::AActor* Other) {
            if (mInstance != 0) {
                Script::FScriptRuntime::Get()->OverlapInstance(mInstance, Begin, Other != nullptr ? Other->GetHandle() : CoreUObject::FObjectHandle{});
            }
        }};

        for (const auto& Component : GetOwner()->GetComponents()) {
            if (auto* Shape{CoreUObject::Cast<Engine::UShapeComponent>(Component.get())}) {
                const Core::Uint32 Begin{Shape->GetOnComponentBeginOverlap().Add([Notify](Engine::UPrimitiveComponent*, Engine::AActor* Other, Engine::UPrimitiveComponent*) {
                    Notify(true, Other);
                })};
                const Core::Uint32 End{Shape->GetOnComponentEndOverlap().Add([Notify](Engine::UPrimitiveComponent*, Engine::AActor* Other, Engine::UPrimitiveComponent*) {
                    Notify(false, Other);
                })};
                mOverlapBindings.push_back({Shape->GetHandle(), Begin, End});
            }
        }
    }

    void Script::UScriptComponent::UnbindOverlapEvents() {
        for (const FOverlapBinding& Binding : mOverlapBindings) {
            if (auto* Shape{CoreUObject::Cast<Engine::UShapeComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(Binding.mShape))}) {
                Shape->GetOnComponentBeginOverlap().RemoveHandler(Binding.mBegin);
                Shape->GetOnComponentEndOverlap().RemoveHandler(Binding.mEnd);
            }
        }

        mOverlapBindings.clear();
    }

    void Script::UScriptComponent::PostEditChangeProperty(const CoreUObject::FPropertyChangedEvent& Event) {
        Engine::UActorComponent::PostEditChangeProperty(Event);

        if (Event.mProperty != nullptr && Event.mProperty->GetName() == "ScriptClass") {
            mFields.clear();
        }
    }

    const Core::FString& Script::UScriptComponent::GetScriptClass() const {
        return mScriptClass;
    }

    const Core::TArray<Script::FScriptField>& Script::UScriptComponent::GetFields() const {
        return mFields;
    }

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

    void Script::UScriptComponent::RemoveField(const Core::FString& Name) {
        std::erase_if(mFields, [&Name](const Script::FScriptField& Field) {
            return Field.mName == Name;
        });
    }

}
