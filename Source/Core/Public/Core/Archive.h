#pragma once

#include "Core/Guid.h"
#include "Core/Math.h"
#include "Core/Math/FTransform.h"

#include <string_view>

enum class EArchiveMode {
    Loading,
    Saving
};

class FArchive {
public:
    explicit FArchive(EArchiveMode Mode);
    virtual ~FArchive();

public:
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
    virtual void Serialize(std::string_view Name, float& Value) = 0;
    virtual void Serialize(std::string_view Name, FString& Value) = 0;

    void Serialize(std::string_view Name, FGuid& Value);
    void Serialize(std::string_view Name, FVector& Value);
    void Serialize(std::string_view Name, FRotator& Value);
    void Serialize(std::string_view Name, FTransform& Value);

private:
    EArchiveMode mMode{};
    FString mError{};
};
