#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Engine/Subsystem/UEngineSubsystem.h"
#include "Engine/UEngine.h"

void UEngineSubsystem::Initialize(UEngine* Engine) {
    if (mInitialized || Engine == nullptr) {
        return;
    }

    ErrorHandler::Report(GetOuter() != Engine, "UEngineSubsystem", "Subsystem owner must match its Outer", ErrorHandler::EErrorLevel::Critical);
    mInitialized = true;
    OnInitialize();
}

void UEngineSubsystem::Deinitialize() {
    if (!mInitialized) {
        return;
    }

    OnDeinitialize();
    mInitialized = false;
}

UEngine* UEngineSubsystem::GetEngine() const {
    return mInitialized ? Cast<UEngine>(GetOuter()) : nullptr;
}

bool UEngineSubsystem::IsInitialized() const {
    return mInitialized;
}

void UEngineSubsystem::OnInitialize() {
}

void UEngineSubsystem::OnDeinitialize() {
}

bool UEngineSubsystem::CanChangeOuter(const UObject* NewOuter) const {
    return mInitialized ? NewOuter == GetOuter() : (NewOuter == nullptr || NewOuter->GetClass()->IsChildOf(UEngine::StaticClass()));
}

UEngineSubsystem::UEngineSubsystem() {
    InitializeClass(StaticClass());
}
