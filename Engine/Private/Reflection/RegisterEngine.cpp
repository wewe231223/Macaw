#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Engine/Subsystem/UEngineSubsystem.h"
#include "Engine/UEngine.h"

void ReflectionModules::RegisterEngine() {
    UEngineSubsystem::StaticClass();
    UEngine::StaticClass();
}
