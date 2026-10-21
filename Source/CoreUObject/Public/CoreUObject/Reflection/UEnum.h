#pragma once

#include "CoreUObject/Reflection/UStruct.h"

#include <initializer_list>

class UEnum final : public UStruct {
public:
    UEnum(FString Name, std::size_t Size);
    ~UEnum() override;

public:
    void AddValues(std::initializer_list<FEnumValue> Values);
    const TArray<FEnumValue>& GetValues() const;

private:
    TArray<FEnumValue> mValues{};
};
