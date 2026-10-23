#pragma once

class AActor;
class UWorld;

enum class EWorldChange {
    ActorAdded,
    ActorRemoving,
    StructureChanged,
    Destroying
};

class IWorldObserver {
public:
    virtual ~IWorldObserver() = default;

public:
    virtual void OnWorldChanged(UWorld& World, EWorldChange Change, AActor* Actor) = 0;
};
