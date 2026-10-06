#pragma once

#include "Core/Common.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FName.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/TypeInfo.h"
#include <cstddef>
#include <new>
#include <type_traits>

class FArchive;
class UObject;

namespace UObjectSystem {
    FObjectHandle Register(UObject* Object);
    void Unregister(UObject* Object, FObjectHandle Handle);
    bool Rename(UObject* Object, FName NewName, UObject* NewOuter);
    bool RestoreGuid(UObject* Object, const FGuid& Guid);
    void DetachInners(UObject* Object);
    bool TryGet(Uint32 Index, FObjectHandle& Out);
    Uint32 GetItemCount();
}

class UObject {
public:
    UObject();
    virtual ~UObject();
    UObject(const UObject&) = delete;
    UObject& operator=(const UObject&) = delete;

public:
    const FGuid& GetGuid() const;
    FObjectHandle GetHandle() const;
    bool RestoreGuid(const FGuid& InGuid);

    FName GetName() const;
    bool SetName(FName InName);
    bool Rename(FName NewName, UObject* NewOuter = nullptr);

    UObject* GetOuter() const;
    UObject* GetOutermost() const;
    UObject* GetTypedOuter(const FTypeInfo* Type) const;
    template <typename T>
    T* GetTypedOuter() const;

    bool IsIn(const UObject* Outer) const;
    bool SetOuter(UObject* InOuter);
    FString GetPathName(const UObject* StopOuter = nullptr) const;

    void Save(FArchive& Archive);
    void Load(FArchive& Archive);

    static void* operator new(std::size_t Size);
    static void operator delete(void* Ptr) noexcept;
    static void* operator new(std::size_t Size, std::align_val_t Alignment);
    static void operator delete(void* Ptr, std::align_val_t Alignment) noexcept;

    JG_DECLARE_ROOT_TYPEINFO(UObject)

protected:
    virtual void Serialize(FArchive& Archive);

private:
    void SetHandle(FObjectHandle InHandle);
    virtual bool CanChangeOuter(const UObject* NewOuter) const;
    virtual void OnIdentityChanged();

    friend FObjectHandle UObjectSystem::Register(UObject* Object);
    friend void UObjectSystem::Unregister(UObject* Object, FObjectHandle Handle);
    friend bool UObjectSystem::Rename(UObject* Object, FName NewName, UObject* NewOuter);
    friend bool UObjectSystem::RestoreGuid(UObject* Object, const FGuid& Guid);
    friend void UObjectSystem::DetachInners(UObject* Object);

private:
    FGuid mGuid{};
    FObjectHandle mHandle{};
    FName mName{};
    UObject* mOuter{};
    TArray<UObject*> mInnerObjects{};
    bool mDestroying{};
};

template <typename T>
T* UObject::GetTypedOuter() const {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::TypeInfoOwner, T>);

    return static_cast<T*>(GetTypedOuter(T::StaticTypeInfo()));
}
