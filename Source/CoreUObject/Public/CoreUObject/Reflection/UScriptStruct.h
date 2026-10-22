#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/UStruct.h"

class UScriptStruct final : public UStruct {
public:
    ReflectTypes(UScriptStruct, UStruct);

public:
    UScriptStruct(FString Name, std::size_t Size, const UScriptStruct* Parent = nullptr);
    ~UScriptStruct() override;

public:
    ReflectStruct(UScriptStruct);

    const UScriptStruct* GetSuperStruct() const;
    bool IsChildOf(const UScriptStruct* Parent) const;

private:
    ReflectMembers();

private:
    const UScriptStruct* mParent{};
    ReflectProperty(mParent, "Parent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
