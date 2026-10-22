#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"
#include "Core/Math.h"

#include <string_view>

enum class EArchiveMode {
    Loading,
    Saving
};

class FArchive {
public:
    ReflectTypes(FArchive);

public:
    explicit FArchive(EArchiveMode Mode);
    virtual ~FArchive();

public:
    ReflectStruct(FArchive);

    bool IsLoading() const;

    bool HasError() const;
    const FString& GetError() const;
    void SetError(const FString& Message);

    virtual bool HasField(std::string_view Name) const = 0;

    virtual bool BeginObject(std::string_view Name) = 0;
    virtual void EndObject() = 0;

    virtual bool BeginArray(std::string_view Name, Uint32& Count) = 0;
    virtual void EndArray() = 0;

    virtual void Serialize(std::string_view Name, bool& Value) = 0;
    virtual void Serialize(std::string_view Name, Int32& Value) = 0;
    virtual void Serialize(std::string_view Name, Uint32& Value) = 0;
    virtual void Serialize(std::string_view Name, Uint64& Value);
    virtual void Serialize(std::string_view Name, float& Value) = 0;
    virtual void Serialize(std::string_view Name, FString& Value) = 0;

    void Serialize(std::string_view Name, FGuid& Value);
    void Serialize(std::string_view Name, FVector2& Value);
    void Serialize(std::string_view Name, FVector& Value);
    void Serialize(std::string_view Name, FVector4& Value);
    void Serialize(std::string_view Name, FQuat& Value);
    void Serialize(std::string_view Name, FRotator& Value);
    void Serialize(std::string_view Name, FMatrix& Value);
    void Serialize(std::string_view Name, FTransform& Value);
    void Serialize(std::string_view Name, FBox& Value);
    void Serialize(std::string_view Name, FRay& Value);

private:
    ReflectMembers();

private:
    EArchiveMode mMode{};
    ReflectProperty(mMode, "Mode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FString mError{};
    ReflectProperty(mError, "Error", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
