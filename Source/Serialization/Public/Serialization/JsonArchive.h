#pragma once

#include "Core/Archive.h"

#include <filesystem>

class FJsonArchive final : public FArchive {
public:
    using FArchive::Serialize;

private:
    struct FImplementation;

public:
    explicit FJsonArchive(EArchiveMode Mode);
    ~FJsonArchive() override;

public:
    bool Load(const std::filesystem::path& Path);
    bool Save(const std::filesystem::path& Path);

    bool Parse(const FString& Text);
    FString ToString() const;

    bool HasField(std::string_view Name) const override;

    bool BeginObject(std::string_view Name) override;
    void EndObject() override;

    bool BeginArray(std::string_view Name, Uint32& Count) override;
    void EndArray() override;

    void Serialize(std::string_view Name, bool& Value) override;
    void Serialize(std::string_view Name, Int32& Value) override;
    void Serialize(std::string_view Name, Uint32& Value) override;
    void Serialize(std::string_view Name, float& Value) override;
    void Serialize(std::string_view Name, FString& Value) override;

private:
    std::unique_ptr<FImplementation> mImplementation{};
};
