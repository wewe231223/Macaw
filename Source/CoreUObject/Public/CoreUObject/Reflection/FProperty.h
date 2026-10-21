#pragma once

#include "CoreUObject/Reflection/FPropertyValue.h"
#include "CoreUObject/Reflection/EPropertyType.h"
#include "CoreUObject/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FEnumValue.h"

class UStruct;
class FArchive;

class FProperty {
public:
    using FReader = FPropertyValue (*)(const void*);
    using FWriter = bool (*)(void*, const FPropertyValue&);

public:
    FProperty(FString Name, EPropertyType Type, EPropertyFlags Flags, FReader Reader, FWriter Writer, const UStruct* Struct = nullptr, TArray<FEnumValue> Values = {}, FWriter Loader = nullptr);

public:
    const FString& GetName() const;
    EPropertyType GetType() const;
    bool HasFlag(EPropertyFlags Flag) const;

    const UStruct* GetStruct() const;
    const TArray<FEnumValue>& GetEnumValues() const;

    FPropertyValue ReadValue(const void* Container) const;
    bool WriteValue(void* Container, const FPropertyValue& Value) const;

    void SerializeValue(void* Container, FArchive& Archive) const;

private:
    bool WriteValue(void* Container, const FPropertyValue& Value, FWriter Writer) const;

private:
    FString mName{};
    EPropertyType mType{};
    EPropertyFlags mFlags{};

    FReader mReader{};
    FWriter mWriter{};
    FWriter mLoader{};

    const UStruct* mStruct{};
    TArray<FEnumValue> mEnumValues{};
};
