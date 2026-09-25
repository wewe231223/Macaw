#pragma once

#include "Core/Archive/FArchive.h"

class FArchiveMemory : public FArchive {
public:
    // Save
    FArchiveMemory(TArray<Uint8>& InBytes);
    // Load
    FArchiveMemory(const TArray<Uint8>& InBytes);

    // Primitives
    virtual void Serialize(std::string_view Name, bool& Value) override;
    virtual void Serialize(std::string_view Name, Uint8& Value) override;
    virtual void Serialize(std::string_view Name, Int32& Value) override;
    virtual void Serialize(std::string_view Name, Uint32& Value) override;
    virtual void Serialize(std::string_view Name, Int64& Value) override;
    virtual void Serialize(std::string_view Name, Uint64& Value) override;
    virtual void Serialize(std::string_view Name, Float32& Value) override;
    virtual void Serialize(std::string_view Name, Float64& Value) override;

    // Engine Core
    virtual void Serialize(std::string_view Name, FString& Value) override;
    virtual void Serialize(std::string_view Name, FGuid& Value) override;

    // Math Type
    virtual void Serialize(std::string_view Name, FVector2D& Value) override;
    virtual void Serialize(std::string_view Name, FVector3& Value) override;
    virtual void Serialize(std::string_view Name, FVector4& Value) override;
    virtual void Serialize(std::string_view Name, FQuat& Value) override;
    virtual void Serialize(std::string_view Name, FMatrix& Value) override;

    // Scope
    virtual void BeginObjectScope(std::string_view Name) override;
    virtual void EndObjectScope() override;
    virtual void BeginArrayScope(std::string_view Name, std::size_t& ArraySize) override;
    virtual void EndArrayScope() override;

private:
    // Save
    TArray<Uint8>* mWriteBytes{};

    // Load
    const TArray<Uint8>* mReadBytes{};
    std::size_t mReadOffset{};
};
