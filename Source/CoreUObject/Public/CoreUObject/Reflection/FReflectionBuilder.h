#pragma once

#include "Core/Types.h"

class UStruct;
class FProperty;

class FReflectionBuilder {
public:
    explicit FReflectionBuilder(UStruct& Owner);

public:
    UStruct* GetOwner() const;
    void AddProperty(std::unique_ptr<FProperty> Property);

private:
    UStruct* mOwner{};
};
