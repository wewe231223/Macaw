#pragma once

#include "Core/Common.h"
#include "Core/Base/FName.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FTransform.h"
#include <functional>
#include <memory>
#include <type_traits>

class UObject;
class UClass;
class UScriptStruct;
class FPropertyValue;

enum class EPropertyKind {
    Unknown,
    Bool,
    Byte,
    Integer,
    Float,
    String,
    Name,
    Guid,
    Vector2,
    Vector3,
    Vector4,
    Transform,
    Struct,
    Enum,
    Object,
    Array
};

struct FNativeType {
    EPropertyKind mKind{};
    std::size_t mSize{};
    std::size_t mAlignment{};
    void* (*mCopy)(const void*){};
    void (*mDestroy)(void*){};
    void (*mVisitReferences)(const void*, const std::function<void(UObject*)>&){};
    UScriptStruct* (*mGetStruct)(){};
    UClass* (*mGetObjectClass)(){};
    const FNativeType* (*mGetElementType)(){};
    UObject* (*mReadObject)(const void*){};
    FPropertyValue (*mMakeObjectValue)(UObject*){};
};

template <typename T>
const FNativeType* GetNativeType();

template <typename T>
void* CopyNativeValue(const void* Value);

template <typename T>
void DestroyNativeValue(void* Value);

template <typename T>
void VisitNativeReferences(const void* Value, const std::function<void(UObject*)>& Visitor);

template <typename T>
UScriptStruct* GetNativeStruct();

template <typename T>
UClass* GetNativeObjectClass();

template <typename T>
const FNativeType* GetNativeElementType();

bool IsNativeObjectCompatible(const UObject* Object, const UClass* Class);

template <typename T>
UObject* ReadNativeObject(const void* Value);

template <typename T>
FPropertyValue MakeNativeObjectValue(UObject* Object);

void VisitScriptStructReferences(const UScriptStruct* Struct, const void* Value, const std::function<void(UObject*)>& Visitor);

class FPropertyValue {
public:
    FPropertyValue() = default;
    template <typename T>
    explicit FPropertyValue(const T& Value);
    ~FPropertyValue();
    FPropertyValue(const FPropertyValue& Other);
    FPropertyValue& operator=(const FPropertyValue& Other);
    FPropertyValue(FPropertyValue&& Other) noexcept;
    FPropertyValue& operator=(FPropertyValue&& Other) noexcept;

public:
    template <typename T>
    bool Is() const;

    template <typename T>
    const T* Get() const;

    const FNativeType* GetType() const;
    const void* GetData() const;
    void* GetMutableData();
    bool IsEmpty() const;
    void Reset();

private:
    const FNativeType* mType{};
    void* mData{};
};

template <typename T>
const FNativeType* GetNativeType() {
    static const FNativeType Type{[]() {
        if constexpr (std::is_same_v<T, bool>) {
            return EPropertyKind::Bool;
        } else if constexpr (std::is_same_v<T, Uint8>) {
            return EPropertyKind::Byte;
        } else if constexpr (std::is_integral_v<T>) {
            return EPropertyKind::Integer;
        } else if constexpr (std::is_floating_point_v<T>) {
            return EPropertyKind::Float;
        } else if constexpr (std::is_same_v<T, FString>) {
            return EPropertyKind::String;
        } else if constexpr (std::is_same_v<T, FName>) {
            return EPropertyKind::Name;
        } else if constexpr (std::is_same_v<T, FGuid>) {
            return EPropertyKind::Guid;
        } else if constexpr (std::is_same_v<T, FVector2>) {
            return EPropertyKind::Vector2;
        } else if constexpr (std::is_same_v<T, FVector3>) {
            return EPropertyKind::Vector3;
        } else if constexpr (std::is_same_v<T, FVector4>) {
            return EPropertyKind::Vector4;
        } else if constexpr (std::is_same_v<T, FTransform>) {
            return EPropertyKind::Transform;
        } else if constexpr (std::is_enum_v<T>) {
            return EPropertyKind::Enum;
        } else if constexpr (std::is_pointer_v<T> || requires(const T& Value) { Value.Get(); } || requires(const T& Value) { Value.get(); }) {
            return EPropertyKind::Object;
        } else if constexpr (requires(const T& Value) { Value.begin(); Value.end(); }) {
            return EPropertyKind::Array;
        } else {
            return EPropertyKind::Struct;
        }
    }(), sizeof(T), alignof(T), &CopyNativeValue<T>, &DestroyNativeValue<T>, &VisitNativeReferences<T>, &GetNativeStruct<T>, &GetNativeObjectClass<T>, &GetNativeElementType<T>, &ReadNativeObject<T>, &MakeNativeObjectValue<T>};

    return &Type;
}

template <typename T>
void* CopyNativeValue(const void* Value) {
    if constexpr (std::is_copy_constructible_v<T> && !requires { typename T::value_type; }) {
        return new T{*static_cast<const T*>(Value)};
    } else if constexpr (requires { typename T::value_type; }) {
        if constexpr (std::is_copy_constructible_v<typename T::value_type>) {
            return new T{*static_cast<const T*>(Value)};
        }
    }

    return nullptr;
}

template <typename T>
void DestroyNativeValue(void* Value) {
    delete static_cast<T*>(Value);
}

template <typename T>
void VisitNativeReferences(const void* Value, const std::function<void(UObject*)>& Visitor) {
    const T& NativeValue{*static_cast<const T*>(Value)};

    if constexpr (std::is_pointer_v<T> && std::is_convertible_v<T, UObject*>) {
        if (NativeValue != nullptr) {
            Visitor(NativeValue);
        }
    } else if constexpr (requires { NativeValue.Get(); }) {
        if constexpr (std::is_convertible_v<decltype(NativeValue.Get()), UObject*>) {
            if (UObject* Object{NativeValue.Get()}) {
                Visitor(Object);
            }
        }
    } else if constexpr (requires { NativeValue.get(); }) {
        if constexpr (std::is_convertible_v<decltype(NativeValue.get()), UObject*>) {
            if (UObject* Object{NativeValue.get()}) {
                Visitor(Object);
            }
        }
    } else if constexpr (!std::is_same_v<T, FString> && requires { NativeValue.begin(); NativeValue.end(); }) {
        for (const auto& Element : NativeValue) {
            VisitNativeReferences<std::remove_cvref_t<decltype(Element)>>(&Element, Visitor);
        }
    } else if constexpr (requires { T::StaticStruct(); }) {
        VisitScriptStructReferences(T::StaticStruct(), Value, Visitor);
    }
}

template <typename T>
UScriptStruct* GetNativeStruct() {
    if constexpr (requires { T::StaticStruct(); }) {
        return T::StaticStruct();
    } else {
        return nullptr;
    }
}

template <typename T>
UClass* GetNativeObjectClass() {
    if constexpr (std::is_pointer_v<T> && std::is_convertible_v<T, UObject*>) {
        return std::remove_pointer_t<T>::StaticClass();
    } else if constexpr (requires(const T& Value) { Value.Get(); }) {
        return GetNativeObjectClass<decltype(std::declval<const T&>().Get())>();
    } else if constexpr (requires(const T& Value) { Value.get(); }) {
        return GetNativeObjectClass<decltype(std::declval<const T&>().get())>();
    } else {
        return nullptr;
    }
}

template <typename T>
const FNativeType* GetNativeElementType() {
    if constexpr (!std::is_same_v<T, FString> && requires(const T& Value) { typename T::value_type; Value.begin(); Value.end(); }) {
        return GetNativeType<typename T::value_type>();
    } else {
        return nullptr;
    }
}

template <typename T>
FPropertyValue::FPropertyValue(const T& Value)
	: mType(GetNativeType<T>()),
	  mData(mType->mCopy(&Value)) {
    if (mData == nullptr) {
        mType = nullptr;
    }
}

template <typename T>
bool FPropertyValue::Is() const {
    return mType == GetNativeType<T>() && mData != nullptr;
}

template <typename T>
const T* FPropertyValue::Get() const {
    return Is<T>() ? static_cast<const T*>(mData) : nullptr;
}

template <typename T>
UObject* ReadNativeObject(const void* Value) {
    if constexpr (std::is_pointer_v<T> && std::is_convertible_v<T, UObject*>) {
        return *static_cast<const T*>(Value);
    } else {
        return nullptr;
    }
}

template <typename T>
FPropertyValue MakeNativeObjectValue(UObject* Object) {
    if constexpr (std::is_pointer_v<T> && std::is_convertible_v<T, UObject*>) {
        if (Object != nullptr && !IsNativeObjectCompatible(Object, std::remove_pointer_t<T>::StaticClass())) {
            return {};
        }

        return FPropertyValue{static_cast<T>(Object)};
    } else {
        return {};
    }
}
