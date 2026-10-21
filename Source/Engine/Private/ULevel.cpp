#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <algorithm>
#include <utility>

ULevel::ULevel() = default;

ULevel::~ULevel() {
    SetWorld(nullptr);
}

ImplementReflection(ULevel, EClassFlags::NoFactory) {
    static_cast<void>(Information);
}

void ULevel::SetWorld(UWorld* World) {
    mWorld = World;

    for (const auto& Actor : mActors) {
        Actor->SetWorld(World);
    }
}

bool ULevel::AddActor(std::unique_ptr<AActor> Actor) {
    if (Actor == nullptr || Actor->GetRegistry() != GetRegistry() || Actor->GetWorld() != nullptr) {
        return false;
    }

    Actor->SetWorld(mWorld);
    mActors.push_back(std::move(Actor));

    return true;
}

bool ULevel::DestroyActor(FObjectHandle Handle) {
    const auto Position{std::find_if(mActors.begin(), mActors.end(), [Handle](const auto& Actor) {
        return Actor->GetHandle() == Handle;
    })};

    if (Position == mActors.end()) {
        return false;
    }

    (*Position)->SetWorld(nullptr);
    mActors.erase(Position);

    return true;
}

const TArray<std::unique_ptr<AActor>>& ULevel::GetActors() const {
    return mActors;
}
