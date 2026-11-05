#include "Script/UScriptComponent.h"
#include "Script/FScriptRuntime.h"
#include "Engine/AActor.h"

namespace Script {

    Script::UScriptComponent::UScriptComponent() = default;

    Script::UScriptComponent::~UScriptComponent() {
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
        if (mInstance != 0) {
            Script::FScriptRuntime* Runtime{Script::FScriptRuntime::Get()};
            Runtime->EndPlayInstance(mInstance);
            Runtime->DestroyInstance(mInstance);
            mInstance = 0;
        }

        Engine::UActorComponent::EndPlay();
    }

}
