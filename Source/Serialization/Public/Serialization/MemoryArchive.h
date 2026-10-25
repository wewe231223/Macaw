#pragma once

#include "Core/Archive.h"

class FMemoryArchive final : public FArchive {
public:
    ReflectTypes(FMemoryArchive, FArchive);
    using FArchive::Serialize;

public:
    explicit FMemoryArchive(TArray<Uint8>& Bytes);
    explicit FMemoryArchive(const TArray<Uint8>& Bytes);

public:
    ReflectStruct(FMemoryArchive);
    std::size_t GetRemainingBytes() const;
    bool HasField(std::string_view Name) const override;
    bool BeginObject(std::string_view Name) override;
    void EndObject() override;
    bool BeginArray(std::string_view Name, Uint32& Count) override;
    void EndArray() override;
    void Serialize(std::string_view Name, bool& Value) override;
    void Serialize(std::string_view Name, Int32& Value) override;
    void Serialize(std::string_view Name, Uint32& Value) override;
    void Serialize(std::string_view Name, Uint64& Value) override;
    void Serialize(std::string_view Name, float& Value) override;
    void Serialize(std::string_view Name, FString& Value) override;

private:
    ReflectMembers();
    void SerializeBytes(void* Value, std::size_t Size);

private:
    TArray<Uint8>* mWriteBytes{};
    ReflectProperty(mWriteBytes, "WriteBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    const TArray<Uint8>* mReadBytes{};
    ReflectProperty(mReadBytes, "ReadBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mReadOffset{};
    ReflectProperty(mReadOffset, "ReadOffset", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
