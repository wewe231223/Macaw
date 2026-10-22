#pragma once

#include "Core/Guid.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectHandle.h"

#include <new>

class FObjectRegistry;
class FArchive;
class FProperty;
struct FPropertyChangedEvent;

class UObject {
public:
    ReflectTypes(UObject);

public:
    UObject();
    virtual ~UObject();
    UObject(const UObject&) = delete;
    UObject& operator=(const UObject&) = delete;
    UObject(UObject&&) = delete;
    UObject& operator=(UObject&&) = delete;

public:
    ReflectRootClass(UObject);

    bool IsA(const UClass* Class) const;
    template <typename T>
    bool IsA() const;

    const FGuid& GetGuid() const;
    FObjectHandle GetHandle() const;

    const FString& GetName() const;
    bool SetName(const FString& Name);
    bool RestoreGuid(const FGuid& Guid);

    FObjectRegistry* GetRegistry() const;

    virtual void Serialize(FArchive& Archive);
    virtual void PreEditChange(const FProperty& Property);
    virtual void PostEditChangeProperty(const FPropertyChangedEvent& Event);

    static void* operator new(std::size_t Size);
    static void* operator new(std::size_t Size, std::align_val_t Alignment);
    static void operator delete(void* Pointer) noexcept;
    static void operator delete(void* Pointer, std::align_val_t Alignment) noexcept;

private:
    ReflectMembers();
    friend class FObjectRegistry;

private:
    FGuid mGuid{};
    ReflectAccessor(mGuid, GetGuid, RestoreGuid, "Id", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly);


    FObjectHandle mHandle{};
    ReflectProperty(mHandle, "Handle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FString mName{"Object"};
    ReflectAccessor(mName, GetName, SetName, "Name", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    FObjectRegistry* mRegistry{};
    ReflectProperty(mRegistry, "Registry", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

#include "CoreUObject/UObjectTemplates.h"
