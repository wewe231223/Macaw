#include "pch.h"
#include "World/Tick/FActorComponentTickFunction.h"
#include "World/Component/UActorComponent.h"
#include "World/AActor.h"

FActorComponentTickFunction::FActorComponentTickFunction(UActorComponent& Target)
	: mTarget{&Target} {
}

bool FActorComponentTickFunction::CanTick(ETickType TickType) const {
    const AActor* Owner{mTarget->GetOwner()};

    return mTarget->IsRegistered() && !mTarget->IsBeingDestroyed() && Owner != nullptr && !Owner->IsBeingDestroyed() && Owner->HasFinishedSpawning() && ShouldTick(TickType, mTarget->HasBegunPlay());
}

void FActorComponentTickFunction::ExecuteTick(float DeltaTime) {
    mTarget->TickComponent(DeltaTime);
}
