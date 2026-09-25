#include "pch.h"
#include "UObject.h"
#include "Memory.h"
#include "Core/Archive/FArchive.h"

UObject::UObject()
    : mGuid(FGuid::NewGuid()),
      mName() {
}

const FGuid& UObject::GetGuid() const {
    return mGuid;
}

FObjectHandle UObject::GetHandle() const {
    return mHandle;
}

void UObject::SetHandle(FObjectHandle InHandle) {
    mHandle = InHandle;
}

void UObject::RestoreGuid(const FGuid& InGuid) {
    mGuid = InGuid;
}

void* UObject::operator new(std::size_t Size) {
    return Memory::Allocate(Size, alignof(std::max_align_t), Memory::EMemoryTag::UObject);
}

void UObject::operator delete(void* Ptr) noexcept {
    Memory::Free(Ptr);
}

void* UObject::operator new(std::size_t Size, std::align_val_t Alignment) {
    return Memory::Allocate(Size, static_cast<std::size_t>(Alignment), Memory::EMemoryTag::UObject);
}

void UObject::operator delete(void* Ptr, std::align_val_t) noexcept {
    Memory::Free(Ptr);
}

FName UObject::GetName() const {
    return mName;
}

void UObject::SetName(FName InName) {
    mName = InName;
}

void UObject::Save(FArchive& Archive) {
    ErrorHandler::Report(Archive.IsSaving() == false, "Save Error", "Given archive is not set as saving mode", ErrorHandler::EErrorLevel::Error);
    Serialize(Archive);
}

void UObject::Load(FArchive& Archive) {
    ErrorHandler::Report(Archive.IsLoading() == false, "Load Error", "Given archive is not set as loading mode", ErrorHandler::EErrorLevel::Error);
    Serialize(Archive);
}

void UObject::Serialize(FArchive& Archive) {
    Archive.Serialize("Guid", mGuid);
    FString TypeNameStr{GetTypeInfo()->mTypeName};
    Archive.Serialize("TypeName", TypeNameStr);
    Archive.Serialize("Name", mName);
}
