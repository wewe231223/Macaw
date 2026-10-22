#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Archive.h"

#include <filesystem>
#include <rapidjson/document.h>

class FJsonArchive final : public FArchive {
public:
    ReflectTypes(FJsonArchive, FArchive);

public:
    using FArchive::Serialize;

public:
    explicit FJsonArchive(EArchiveMode Mode);
    ~FJsonArchive() override;
    FJsonArchive(const FJsonArchive&) = delete;
    FJsonArchive& operator=(const FJsonArchive&) = delete;

public:
    ReflectStruct(FJsonArchive);

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
    void Serialize(std::string_view Name, Uint64& Value) override;
    void Serialize(std::string_view Name, float& Value) override;
    void Serialize(std::string_view Name, FString& Value) override;

private:
    rapidjson::Value* Find(std::string_view Name, bool Create);

private:
    ReflectMembers();

private:
    rapidjson::Document mDocument{};
    ReflectProperty(mDocument, "Document", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<rapidjson::Value*> mStack{};
    ReflectProperty(mStack, "Stack", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
