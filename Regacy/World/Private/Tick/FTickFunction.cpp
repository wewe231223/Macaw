#include "pch.h"
#include "World/Tick/FTickFunction.h"

bool FTickFunction::CanEverTick() const {
    return mCanEverTick;
}

void FTickFunction::SetCanEverTick(bool CanEverTick) {
    mCanEverTick = CanEverTick;
}

bool FTickFunction::IsTickEnabled() const {
    return mTickEnabled;
}

void FTickFunction::SetTickEnabled(bool TickEnabled) {
    mTickEnabled = TickEnabled;
}

bool FTickFunction::IsTickInEditor() const {
    return mTickInEditor;
}

void FTickFunction::SetTickInEditor(bool TickInEditor) {
    mTickInEditor = TickInEditor;
}

bool FTickFunction::IsTickWhenPaused() const {
    return mTickWhenPaused;
}

void FTickFunction::SetTickWhenPaused(bool TickWhenPaused) {
    mTickWhenPaused = TickWhenPaused;
}

bool FTickFunction::ShouldTick(ETickType TickType, bool HasBegunPlay) const {
    if (!mCanEverTick || !mTickEnabled) {
        return false;
    }

    switch (TickType) {
    case ETickType::Editor:
        return mTickInEditor;
    case ETickType::Game:
        return HasBegunPlay;
    case ETickType::Paused:
        return HasBegunPlay && mTickWhenPaused;
    default:
        return false;
    }
}
