#include "UObject.h"

UObject::UObject()
	: Guid(FGuid::NewGuid())
{
}

const FGuid& UObject::GetGuid() const
{
	return Guid;
}

FObjectHandle UObject::GetHandle() const
{
	return Handle;
}

void UObject::SetHandle(FObjectHandle InHandle)
{
	Handle = InHandle;
}

void UObject::RestoreGuid(const FGuid& InGuid)
{
	Guid = InGuid;
}