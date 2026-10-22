#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/UStruct.h"

#include <initializer_list>

class UEnum final : public UStruct {
public:
    ReflectTypes(UEnum, UStruct);

public:
    UEnum(FString Name, std::size_t Size);
    ~UEnum() override;

public:
    ReflectStruct(UEnum);

    void AddValues(std::initializer_list<FEnumValue> Values);
    const TArray<FEnumValue>& GetValues() const;

private:
    ReflectMembers();

private:
    TArray<FEnumValue> mValues{};
    ReflectProperty(mValues, "Values", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
