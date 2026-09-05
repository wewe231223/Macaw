#pragma once

#include <cstdint>
#include "Core/Base/FObjectHandle.h"

class UObject;
struct FGuid;

namespace UObjectSystem
{
	FObjectHandle Register(UObject* Object);
	
	void Unregister(UObject* Object, FObjectHandle Handle);

	UObject* Resolve(FObjectHandle Handle);

	FObjectHandle FindHandleByGuid(const FGuid& Guid);

	FObjectHandle GetHandle(const UObject* Object);

	std::uint32_t GetObjectCount();
}
