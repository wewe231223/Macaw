#include "pch.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/UFunction.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/UObjectSystem.h"
#include "Core/Memory/Memory.h"
#include "Core/Archive/FArchive.h"

UObject::UObject()
	: mClass(FReflectionRegistry::IsBootstrapping() ? nullptr : FObjectInitializer::GetConstructingClass()),
	  mClassDefaultObject(!FReflectionRegistry::IsBootstrapping() && FObjectInitializer::IsConstructingDefault()),
	  mGuid(FGuid::NewGuid()) {
    if (!FReflectionRegistry::IsBootstrapping() && mClass == nullptr) {
        mClass = StaticClass();
    }
}

UObject::~UObject() {
    mDestroying = true;
    UObjectSystem::Unregister(this, mHandle);
    UObjectSystem::DetachInners(this);
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

bool UObject::RestoreGuid(const FGuid& InGuid) {
    return UObjectSystem::RestoreGuid(this, InGuid);
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

bool UObject::SetName(FName InName) {
    return Rename(InName);
}

bool UObject::Rename(FName NewName, UObject* NewOuter) {
    return UObjectSystem::Rename(this, NewName, NewOuter != nullptr ? NewOuter : mOuter);
}

UObject* UObject::GetOuter() const {
    return mOuter;
}

UObject* UObject::GetOutermost() const {
    const UObject* Object{this};

    while (Object->GetOuter() != nullptr) {
        Object = Object->GetOuter();
    }

    return const_cast<UObject*>(Object);
}

UObject* UObject::GetTypedOuter(const UClass* Type) const {
    for (UObject* Object{mOuter}; Object != nullptr; Object = Object->GetOuter()) {
        if (Object->GetClass()->IsChildOf(Type)) {
            return Object;
        }
    }

    return nullptr;
}

bool UObject::IsIn(const UObject* Outer) const {
    for (const UObject* Object{mOuter}; Object != nullptr; Object = Object->GetOuter()) {
        if (Object == Outer) {
            return true;
        }
    }

    return Outer == nullptr;
}

bool UObject::SetOuter(UObject* InOuter) {
    return UObjectSystem::Rename(this, mName, InOuter);
}

FString UObject::GetPathName(const UObject* StopOuter) const {
    FString Path{};

    for (const UObject* Object{this}; Object != nullptr && Object != StopOuter; Object = Object->GetOuter()) {
        const FString Name{Object->GetName().ToString()};

        Path = Path.empty() ? Name : Name + "." + Path;
    }

    return Path;
}

bool UObject::CanChangeOuter(const UObject* NewOuter) const {
    return true;
}

void UObject::OnIdentityChanged() {
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
    FGuid Guid{mGuid};
    FName Name{mName};
    FName TypeName{GetClass()->GetClassName()};

    Archive.Serialize("Guid", Guid);
    Archive.Serialize("TypeName", TypeName);
    Archive.Serialize("Name", Name);
    GetClass()->SerializeProperties(this, Archive);

    if (Archive.IsLoading()) {
        ErrorHandler::Report(!RestoreGuid(Guid), "UObject", "Cannot restore an object with a conflicting GUID", ErrorHandler::EErrorLevel::Error);
        ErrorHandler::Report(!SetName(Name), "UObject", "Cannot restore an object with a conflicting name", ErrorHandler::EErrorLevel::Error);
    }
}

UClass* UObject::GetClass() const noexcept {
    return mClass;
}

bool UObject::IsClassDefaultObject() const noexcept {
    return mClassDefaultObject;
}

bool UObject::IsA(const UClass* Class) const noexcept {
    return mClass != nullptr && mClass->IsChildOf(Class);
}

const UFunction* UObject::FindFunction(FName Name) const {
    return mClass != nullptr ? mClass->FindFunction(Name) : nullptr;
}

bool UObject::ProcessEvent(const UFunction* Function, std::span<const FPropertyValue> Arguments, FPropertyValue& Result) {
    if (Function == nullptr) {
        Result.Reset();
        return false;
    }

    return Function->Invoke(*this, Arguments, Result);
}

void UObject::InitializeClass(UClass* Class) {
    if (UClass* ConstructingClass{FObjectInitializer::GetConstructingClass()}; !FReflectionRegistry::IsBootstrapping() && ConstructingClass != nullptr && ConstructingClass->IsChildOf(Class)) {
        mClass = ConstructingClass;
    } else {
        mClass = Class;
    }
}
