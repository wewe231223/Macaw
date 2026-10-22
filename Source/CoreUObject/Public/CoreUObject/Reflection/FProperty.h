#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "CoreUObject/Reflection/EPropertyType.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FEnumValue.h"

class UStruct;
class FArchive;
struct FNativeType;

class FProperty {
public:
    ReflectTypes(FProperty);

public:
    using FReader = FPropertyValue (*)(const void*);
    using FWriter = bool (*)(void*, const FPropertyValue&);

public:
    FProperty(FString Name, EPropertyType Type, EPropertyFlags Flags, FReader Reader, FWriter Writer, const UStruct* Struct = nullptr, TArray<FEnumValue> Values = {}, FWriter Loader = nullptr, const FNativeType* NativeType = nullptr);

public:
    ReflectStruct(FProperty);

    const FString& GetName() const;
    EPropertyType GetType() const;
    const FNativeType* GetNativeType() const;
    bool HasFlag(EPropertyFlags Flag) const;

    const UStruct* GetStruct() const;
    const TArray<FEnumValue>& GetEnumValues() const;

    FPropertyValue ReadValue(const void* Container) const;
    bool WriteValue(void* Container, const FPropertyValue& Value) const;

    void SerializeValue(void* Container, FArchive& Archive) const;

private:
    bool WriteValue(void* Container, const FPropertyValue& Value, FWriter Writer) const;

private:
    ReflectMembers();

private:
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    EPropertyType mType{};
    ReflectProperty(mType, "Type", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    EPropertyFlags mFlags{};
    ReflectProperty(mFlags, "Flags", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FReader mReader{};
    ReflectProperty(mReader, "Reader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FWriter mWriter{};
    ReflectProperty(mWriter, "Writer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FWriter mLoader{};
    ReflectProperty(mLoader, "Loader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    const UStruct* mStruct{};
    ReflectProperty(mStruct, "Struct", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    TArray<FEnumValue> mEnumValues{};
    ReflectProperty(mEnumValues, "EnumValues", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    const FNativeType* mNativeType{};
    ReflectProperty(mNativeType, "NativeType", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
