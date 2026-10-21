#include "Engine/FWorldContext.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "CoreUObject/FObjectRegistry.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/Reflection/RegisterEngine.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <utility>

FWorldContext::FWorldContext() {
    ReflectionModules::RegisterEngine();

    mWorld = NewObject<UWorld>(mRegistry);

    if (mWorld == nullptr) {
        ErrorHandler::Fatal("World", "World creation failed");
    }

    mWorld->SetName("EditorWorld");

    auto Level{NewObject<ULevel>(mRegistry)};

    if (Level == nullptr) {
        ErrorHandler::Fatal("World", "Persistent level creation failed");
    }

    Level->SetName("PersistentLevel");

    if (!mWorld->SetPersistentLevel(std::move(Level))) {
        ErrorHandler::Fatal("World", "Persistent level initialization failed");
    }
}

FWorldContext::~FWorldContext() = default;

FObjectRegistry& FWorldContext::GetRegistry() {
    return mRegistry;
}

const FObjectRegistry& FWorldContext::GetRegistry() const {
    return mRegistry;
}

UWorld* FWorldContext::GetWorld() const {
    return mWorld.get();
}
