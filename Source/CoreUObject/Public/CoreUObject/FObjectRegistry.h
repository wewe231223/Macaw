#pragma once

#include "Core/GuidHash.h"
#include "CoreUObject/FObjectHandle.h"

class UObject;

class FObjectRegistry {
private:
    struct FSlot {
        UObject* mObject{};
        Uint32 mGeneration{1};
    };

public:
    FObjectRegistry();
    ~FObjectRegistry();
    FObjectRegistry(const FObjectRegistry&) = delete;
    FObjectRegistry& operator=(const FObjectRegistry&) = delete;

public:
    bool Register(UObject& Object);
    void Unregister(UObject& Object);

    bool RestoreGuid(UObject& Object, const FGuid& Guid);

    UObject* Resolve(FObjectHandle Handle) const;
    UObject* Find(const FGuid& Guid) const;

    std::size_t GetObjectCount() const;

private:
    TArray<FSlot> mSlots{};
    TArray<Uint32> mFreeSlots{};

    TMap<FGuid, UObject*, FGuidHash> mObjectsByGuid{};
};
