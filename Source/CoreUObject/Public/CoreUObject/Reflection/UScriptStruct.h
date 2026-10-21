#pragma once

#include "CoreUObject/Reflection/UStruct.h"

class UScriptStruct final : public UStruct {
public:
    UScriptStruct(FString Name, std::size_t Size);
    ~UScriptStruct() override;
};
