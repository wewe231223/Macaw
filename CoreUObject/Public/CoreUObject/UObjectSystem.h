#pragma once

#include "Core/CoreMinimal.h"
#include "CoreUObject/FObjectHandle.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FName.h"
#include "CoreUObject/UObject.h"
#include <string_view>

namespace UObjectSystem {
    FObjectHandle Register(UObject* Object);
    FObjectHandle RegisterWithGuid(UObject* Object, const FGuid& InGuid);
    void Unregister(UObject* Object, FObjectHandle Handle);

    UObject* Resolve(FObjectHandle Handle);
    FObjectHandle FindHandleByGuid(const FGuid& Guid);
    FObjectHandle GetHandle(const UObject* Object);

    UObject* FindObject(const UObject* Outer, FName Name);
    UObject* FindObjectByPath(std::string_view Path, const UObject* Outer = nullptr);
    TArray<UObject*> GetObjectsWithOuter(const UObject* Outer, bool IncludeNested = false);
    FName MakeUniqueObjectName(const UObject* Outer, FName BaseName, const UObject* IgnoreObject = nullptr);
    bool IsValidObjectName(FName Name);

    bool Rename(UObject* Object, FName NewName, UObject* NewOuter);
    bool RestoreGuid(UObject* Object, const FGuid& Guid);
    void DetachInners(UObject* Object);

    Uint32 GetObjectCount();
}
