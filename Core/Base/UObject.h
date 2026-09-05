#pragma once

#include "FGuid.h"
#include "FObjectHandle.h"
#include "TypeInfo.h"

#include <cstddef>
#include <new>

class UObject;

namespace UObjectSystem
{
	FObjectHandle Register(UObject* Object);
}

class UObject
{
public:
	UObject();
	virtual ~UObject() = default;

	UObject(const UObject&) = delete;
	UObject& operator=(const UObject&) = delete;

	const FGuid& GetGuid() const;
	FObjectHandle GetHandle() const;

	static void* operator new(std::size_t Size);
	static void operator delete(void* Ptr) noexcept;

	static void* operator new(std::size_t Size, std::align_val_t Alignment);
	static void operator delete(void* Ptr, std::align_val_t Alignment) noexcept;

	// RTTI
	//static const FTypeInfo* StaticTypeInfo();
	//virtual const FTypeInfo* GetTypeInfo() const = 0;


private:
	friend FObjectHandle UObjectSystem::Register(UObject* Object);

	void SetHandle(FObjectHandle InHandle);

	void RestoreGuid(const FGuid& InGuid);


private:
	FGuid Guid;
	FObjectHandle Handle;
};
