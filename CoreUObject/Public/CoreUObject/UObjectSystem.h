#pragma once

#include "Core/CoreMinimal.h"

#include <cstdint>
#include "CoreUObject/FObjectHandle.h"
#include "Core/Base/FGuid.h"
#include "CoreUObject/UObject.h"

namespace UObjectSystem {
    FObjectHandle Register(UObject* Object);

    FObjectHandle RegisterWithGuid(UObject* Object, const FGuid& InGuid);

    void Unregister(UObject* Object, FObjectHandle Handle);

    UObject* Resolve(FObjectHandle Handle);

    FObjectHandle FindHandleByGuid(const FGuid& Guid);

    FObjectHandle GetHandle(const UObject* Object);

    std::uint32_t GetObjectCount();
}
