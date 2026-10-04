#pragma once

#include <filesystem>
#include <rapidjson/document.h>

class FJsonFile final {
public:
    static bool Load(const std::filesystem::path& FilePath, rapidjson::Document& Document);
    static bool Save(const std::filesystem::path& FilePath, const rapidjson::Document& Document);
};
