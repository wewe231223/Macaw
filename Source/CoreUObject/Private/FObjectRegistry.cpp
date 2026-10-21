#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/UObject.h"

FObjectRegistry::FObjectRegistry() = default;

FObjectRegistry::~FObjectRegistry() = default;

bool FObjectRegistry::Register(UObject& Object) {
    if (Object.mRegistry != nullptr || mObjectsByGuid.contains(Object.mGuid)) {
        return false;
    }

    Uint32 Index{};

    if (mFreeSlots.empty()) {
        Index = static_cast<Uint32>(mSlots.size());
        mSlots.push_back(FSlot{});
    } else {
        Index = mFreeSlots.back();
        mFreeSlots.pop_back();
    }

    FSlot& Slot{mSlots[Index]};

    mObjectsByGuid.emplace(Object.mGuid, &Object);
    Slot.mObject = &Object;

    Object.mHandle = {Index, Slot.mGeneration};
    Object.mRegistry = this;

    return true;
}

void FObjectRegistry::Unregister(UObject& Object) {
    if (Object.mRegistry != this || Resolve(Object.mHandle) != &Object) {
        return;
    }

    FSlot& Slot{mSlots[Object.mHandle.mIndex]};

    mObjectsByGuid.erase(Object.mGuid);
    Slot.mObject = nullptr;
    ++Slot.mGeneration;

    if (Slot.mGeneration == 0) {
        ++Slot.mGeneration;
    }

    mFreeSlots.push_back(Object.mHandle.mIndex);

    Object.mHandle = {};
    Object.mRegistry = nullptr;
}

bool FObjectRegistry::RestoreGuid(UObject& Object, const FGuid& Guid) {
    if (!Guid.IsValid() || Object.mRegistry != this) {
        return false;
    }

    if (const auto Position{mObjectsByGuid.find(Guid)}; Position != mObjectsByGuid.end() && Position->second != &Object) {
        return false;
    }

    mObjectsByGuid.erase(Object.mGuid);
    Object.mGuid = Guid;
    mObjectsByGuid.emplace(Guid, &Object);

    return true;
}

UObject* FObjectRegistry::Resolve(FObjectHandle Handle) const {
    return Handle.mIndex < mSlots.size() && mSlots[Handle.mIndex].mGeneration == Handle.mGeneration ? mSlots[Handle.mIndex].mObject : nullptr;
}

UObject* FObjectRegistry::Find(const FGuid& Guid) const {
    const auto Position{mObjectsByGuid.find(Guid)};
    return Position != mObjectsByGuid.end() ? Position->second : nullptr;
}

std::size_t FObjectRegistry::GetObjectCount() const {
    return mObjectsByGuid.size();
}
