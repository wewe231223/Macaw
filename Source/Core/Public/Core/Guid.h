#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

struct FGuid {
public:
    ReflectTypes(FGuid);

public:
    FGuid();
    ~FGuid() = default;
    FGuid(const FGuid& Other) = default;
    FGuid& operator=(const FGuid& Other) = default;
    FGuid(FGuid&& Other) noexcept = default;
    FGuid& operator=(FGuid&& Other) noexcept = default;

public:
    ReflectStruct(FGuid);

    static bool Parse(const FString& Text, FGuid& Result);
    FString ToString() const;
    bool IsValid() const;
    bool operator==(const FGuid& Other) const;

private:
    ReflectMembers();

public:
    std::array<Uint8, 16> mBytes{};
    ReflectProperty(mBytes, "Bytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
