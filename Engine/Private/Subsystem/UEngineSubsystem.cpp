#include "pch.h"
#include "Engine/Subsystem/UEngineSubsystem.h"

void UEngineSubsystem::Initialize(UEngine* Engine) {
    if (mEngine != nullptr || Engine == nullptr) {
        return;
    }
    mEngine = Engine;
    OnInitialize();
}

void UEngineSubsystem::Deinitialize() {
    if (mEngine == nullptr) {
        return;
    }
    OnDeinitialize();
    mEngine = nullptr;
}

UEngine* UEngineSubsystem::GetEngine() const {
    return mEngine;
}

bool UEngineSubsystem::IsInitialized() const {
    return mEngine != nullptr;
}

void UEngineSubsystem::OnInitialize() {
}

void UEngineSubsystem::OnDeinitialize() {
}
