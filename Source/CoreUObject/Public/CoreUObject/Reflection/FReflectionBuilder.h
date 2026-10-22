#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class UStruct;
class FProperty;

class FReflectionBuilder {
public:
    ReflectTypes(FReflectionBuilder);

public:
    explicit FReflectionBuilder(UStruct& Owner);

public:
    ReflectStruct(FReflectionBuilder);

    UStruct* GetOwner() const;
    void AddProperty(std::unique_ptr<FProperty> Property);

private:
    ReflectMembers();

private:
    UStruct* mOwner{};
    ReflectProperty(mOwner, "Owner", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
