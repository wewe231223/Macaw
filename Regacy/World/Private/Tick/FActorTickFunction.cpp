#include "pch.h"
#include "World/Tick/FActorTickFunction.h"
#include "World/AActor.h"

FActorTickFunction::FActorTickFunction(AActor& Target)
	: mTarget{&Target} {
}

bool FActorTickFunction::CanTick(ETickType TickType) const {
    return mTarget->GetWorld() != nullptr && !mTarget->IsBeingDestroyed() && mTarget->HasFinishedSpawning() && ShouldTick(TickType, mTarget->HasBegunPlay());
}

void FActorTickFunction::ExecuteTick(float DeltaTime) {
    mTarget->Tick(DeltaTime);
}
