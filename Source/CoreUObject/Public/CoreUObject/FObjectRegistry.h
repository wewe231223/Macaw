#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/GuidHash.h"
#include "CoreUObject/FObjectHandle.h"

class UObject;

class FObjectRegistry {
public:
    ReflectTypes(FObjectRegistry);

private:
    struct FSlot {
    public:
        ReflectTypes(FSlot);

    public:
        ReflectStruct(FSlot);

    private:
        ReflectMembers();

    public:
        UObject* mObject{};
        ReflectProperty(mObject, "Object", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

        Uint32 mGeneration{1};
        ReflectProperty(mGeneration, "Generation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    FObjectRegistry();
    ~FObjectRegistry();
    FObjectRegistry(const FObjectRegistry&) = delete;
    FObjectRegistry& operator=(const FObjectRegistry&) = delete;

public:
    ReflectStruct(FObjectRegistry);

    bool Register(UObject& Object);
    void Unregister(UObject& Object);

    bool RestoreGuid(UObject& Object, const FGuid& Guid);

    UObject* Resolve(FObjectHandle Handle) const;
    UObject* Find(const FGuid& Guid) const;

    std::size_t GetObjectCount() const;

private:
    ReflectMembers();

private:
    TArray<FSlot> mSlots{};
    ReflectProperty(mSlots, "Slots", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    TArray<Uint32> mFreeSlots{};
    ReflectProperty(mFreeSlots, "FreeSlots", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    TMap<FGuid, UObject*, FGuidHash> mObjectsByGuid{};
    ReflectProperty(mObjectsByGuid, "ObjectsByGuid", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
