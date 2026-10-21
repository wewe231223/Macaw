#include "CoreUObject/UObject.h"
#include "Core/Archive.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/Memory.h"

UObject::UObject()
	: mGuid{FGuid::NewGuid()} {
    FMemory::ObjectCreated();
}

UObject::~UObject() {
    if (mRegistry != nullptr) {
        mRegistry->Unregister(*this);
    }

    FMemory::ObjectDestroyed();
}

ImplementRootReflection(UObject, EClassFlags::NoFactory) {
    ReflectAccessor(mName, GetName, SetName, "Name", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    ReflectAccessor(mGuid, GetGuid, RestoreGuid, "Id", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly);
}

bool UObject::IsA(const UClass* Class) const {
    return GetClass()->IsChildOf(Class);
}

const FGuid& UObject::GetGuid() const {
    return mGuid;
}

FObjectHandle UObject::GetHandle() const {
    return mHandle;
}

const FString& UObject::GetName() const {
    return mName;
}

bool UObject::SetName(const FString& Name) {
    if (Name.empty() || Name.size() > 255 || Name.find('\0') != FString::npos) {
        return false;
    }

    mName = Name;

    return true;
}

bool UObject::RestoreGuid(const FGuid& Guid) {
    if (!Guid.IsValid()) {
        return false;
    }

    if (mRegistry != nullptr) {
        return mRegistry->RestoreGuid(*this, Guid);
    }

    mGuid = Guid;

    return true;
}

FObjectRegistry* UObject::GetRegistry() const {
    return mRegistry;
}

void UObject::Serialize(FArchive& Archive) {
    GetClass()->SerializeClassProperties(this, Archive);
}

void UObject::PreEditChange(const FProperty& Property) {
    static_cast<void>(Property);
}

void UObject::PostEditChangeProperty(const FPropertyChangedEvent& Event) {
    static_cast<void>(Event);
}

void* UObject::operator new(std::size_t Size) {
    return FMemory::Allocate(Size, alignof(std::max_align_t));
}

void* UObject::operator new(std::size_t Size, std::align_val_t Alignment) {
    return FMemory::Allocate(Size, static_cast<std::size_t>(Alignment));
}

void UObject::operator delete(void* Pointer) noexcept {
    FMemory::Free(Pointer);
}

void UObject::operator delete(void* Pointer, std::align_val_t Alignment) noexcept {
    static_cast<void>(Alignment);
    FMemory::Free(Pointer);
}
