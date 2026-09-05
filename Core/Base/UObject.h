#pragma once

#include "FGuid.h"
#include "FObjectHandle.h"

class UObject;

namespace ObjectSystem
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


// test
/////
public:
	unsigned char Data = 3;
/////


private:
	friend FObjectHandle ObjectSystem::Register(UObject* Object);

	void SetHandle(FObjectHandle InHandle);

	void RestoreGuid(const FGuid& InGuid);


private:
	FGuid Guid;
	FObjectHandle Handle;
};