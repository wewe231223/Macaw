#pragma once

#include "Core/Guid.h"
#include "CoreUObject/FObjectHandle.h"

#include <cstddef>
#include <memory>
#include <type_traits>

namespace CoreUObject {

    class UObject;
    class UClass;
    struct FObjectReference;

    namespace ObjectRefPrivate {
        CoreUObject::UObject* Resolve(CoreUObject::FObjectHandle Handle, const CoreUObject::UClass* Class);
        Core::FGuid GetGuid(CoreUObject::FObjectHandle Handle);
    }

    template <typename T>
    class TObjectRef {
    public:
        using FObjectType = T;

    public:
        TObjectRef() = default;
        TObjectRef(std::nullptr_t);
        TObjectRef(T* Object);
        ~TObjectRef() = default;

        TObjectRef(const CoreUObject::TObjectRef<T>& Other) = default;
        CoreUObject::TObjectRef<T>& operator=(const CoreUObject::TObjectRef<T>& Other) = default;
        TObjectRef(CoreUObject::TObjectRef<T>&& Other) noexcept = default;
        CoreUObject::TObjectRef<T>& operator=(CoreUObject::TObjectRef<T>&& Other) noexcept = default;

        template <typename U> requires std::is_convertible_v<U*, T*>
        TObjectRef(const CoreUObject::TObjectRef<U>& Other);

    public:
        T* Get() const;
        bool IsValid() const;
        void Reset();

        CoreUObject::FObjectHandle GetHandle() const;
        const Core::FGuid& GetGuid() const;
        static CoreUObject::TObjectRef<T> FromHandle(CoreUObject::FObjectHandle Handle);

        T* operator->() const;
        T& operator*() const;
        operator T*() const;
        explicit operator bool() const;

        bool operator==(std::nullptr_t) const;
        template <typename U> requires (std::is_convertible_v<U*, const T*> || std::is_convertible_v<T*, const U*>)
        bool operator==(U* Object) const;

        template <typename U> requires (std::is_convertible_v<const U*, const T*> || std::is_convertible_v<const T*, const U*>)
        bool operator==(const CoreUObject::TObjectRef<U>& Other) const;

    private:
        template <typename U>
        friend class TObjectRef;
        friend struct FObjectReference;

    private:
        CoreUObject::FObjectHandle mHandle{};
        Core::FGuid mGuid{Core::FGuid::Null()};
    };

    template <typename T>
    struct TObjectRefHash {
    public:
        std::size_t operator()(const CoreUObject::TObjectRef<T>& Reference) const;
    };

    template <typename T>
    struct TIsObjectRef : std::false_type {
    };

    template <typename T>
    struct TIsObjectRef<CoreUObject::TObjectRef<T>> : std::true_type {
    public:
        using FObjectType = T;
    };

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(T* Object);

    template <typename T, typename TDeleter>
    CoreUObject::TObjectRef<T> MakeObjectRef(const std::unique_ptr<T, TDeleter>& Object);

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(const std::shared_ptr<T>& Object);

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(const CoreUObject::TObjectRef<T>& Object);


    template <typename T>
    TObjectRef<T>::TObjectRef(std::nullptr_t) {
    }

    template <typename T>
    TObjectRef<T>::TObjectRef(T* Object) {
        if (Object != nullptr) {
            mHandle = Object->GetHandle();
            mGuid = Object->GetGuid();
        }
    }

    template <typename T>
    template <typename U> requires std::is_convertible_v<U*, T*>
    TObjectRef<T>::TObjectRef(const CoreUObject::TObjectRef<U>& Other)
		: mHandle{Other.mHandle},
		  mGuid{Other.mGuid} {
    }

    template <typename T>
    T* TObjectRef<T>::Get() const {
        using FMutableType = std::remove_const_t<T>;
        static_assert(std::is_base_of_v<CoreUObject::UObject, FMutableType>);
        static_assert(std::is_same_v<typename FMutableType::ReflectionOwner, FMutableType>);

        return static_cast<T*>(CoreUObject::ObjectRefPrivate::Resolve(mHandle, FMutableType::StaticClass()));
    }

    template <typename T>
    bool TObjectRef<T>::IsValid() const {
        return Get() != nullptr;
    }

    template <typename T>
    void TObjectRef<T>::Reset() {
        mHandle = {};
        mGuid = Core::FGuid::Null();
    }

    template <typename T>
    CoreUObject::FObjectHandle TObjectRef<T>::GetHandle() const {
        return mHandle;
    }

    template <typename T>
    const Core::FGuid& TObjectRef<T>::GetGuid() const {
        return mGuid;
    }

    template <typename T>
    CoreUObject::TObjectRef<T> TObjectRef<T>::FromHandle(CoreUObject::FObjectHandle Handle) {
        CoreUObject::TObjectRef<T> Reference{};
        Reference.mHandle = Handle;
        Reference.mGuid = CoreUObject::ObjectRefPrivate::GetGuid(Handle);

        return Reference;
    }

    template <typename T>
    T* TObjectRef<T>::operator->() const {
        return Get();
    }

    template <typename T>
    T& TObjectRef<T>::operator*() const {
        return *Get();
    }

    template <typename T>
    TObjectRef<T>::operator T*() const {
        return Get();
    }

    template <typename T>
    TObjectRef<T>::operator bool() const {
        return IsValid();
    }

    template <typename T>
    bool TObjectRef<T>::operator==(std::nullptr_t) const {
        return Get() == nullptr;
    }

    template <typename T>
    template <typename U> requires (std::is_convertible_v<U*, const T*> || std::is_convertible_v<T*, const U*>)
    bool TObjectRef<T>::operator==(U* Object) const {
        return Get() == Object;
    }

    template <typename T>
    template <typename U> requires (std::is_convertible_v<const U*, const T*> || std::is_convertible_v<const T*, const U*>)
    bool TObjectRef<T>::operator==(const CoreUObject::TObjectRef<U>& Other) const {
        return mHandle == Other.mHandle && (mHandle.IsValid() || mGuid == Other.mGuid);
    }

    template <typename T>
    std::size_t TObjectRefHash<T>::operator()(const CoreUObject::TObjectRef<T>& Reference) const {
        return CoreUObject::FObjectHandleHash{}(Reference.GetHandle());
    }

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(T* Object) {
        return CoreUObject::TObjectRef<T>{Object};
    }

    template <typename T, typename TDeleter>
    CoreUObject::TObjectRef<T> MakeObjectRef(const std::unique_ptr<T, TDeleter>& Object) {
        return CoreUObject::TObjectRef<T>{Object.get()};
    }

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(const std::shared_ptr<T>& Object) {
        return CoreUObject::TObjectRef<T>{Object.get()};
    }

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(const CoreUObject::TObjectRef<T>& Object) {
        return Object;
    }

}
