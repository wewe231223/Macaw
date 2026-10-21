#include "pch.h"
#include "Serialization/FJsonFile.h"
#include "Core/Base/FGuid.h"
#include <fstream>
#include <limits>
#include <windows.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

bool FJsonFile::Load(const std::filesystem::path& FilePath, rapidjson::Document& Document) {
    std::ifstream Input{FilePath, std::ios::binary};

    if (!Input.is_open()) {
        return false;
    }

    rapidjson::IStreamWrapper Stream{Input};

    Document.ParseStream<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag>(Stream);

    return !Input.bad() && !Document.HasParseError() && Document.IsObject();
}

bool FJsonFile::Save(const std::filesystem::path& FilePath, const rapidjson::Document& Document) {
    if (FilePath.empty() || !Document.IsObject()) {
        return false;
    }

    rapidjson::StringBuffer Buffer{};
    rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer{Buffer};

    if (!Document.Accept(Writer)) {
        return false;
    }

    std::error_code Error{};
    const std::filesystem::path TargetPath{std::filesystem::absolute(FilePath, Error)};

    if (Error) {
        return false;
    }

    std::filesystem::create_directories(TargetPath.parent_path(), Error);

    if (Error) {
        return false;
    }

    std::filesystem::path TemporaryPath{TargetPath};

    TemporaryPath += "." + std::string{FGuid::NewGuid().ToString().c_str()} + ".tmp";

    const HANDLE File{CreateFileW(TemporaryPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)};

    if (File == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool Saved{true};
    std::size_t Offset{};

    while (Offset < Buffer.GetSize()) {
        const DWORD Size{static_cast<DWORD>(std::min<std::size_t>(Buffer.GetSize() - Offset, std::numeric_limits<DWORD>::max()))};
        DWORD Written{};

        if (!WriteFile(File, Buffer.GetString() + Offset, Size, &Written, nullptr) || Written == 0) {
            Saved = false;
            break;
        }

        Offset += Written;
    }

    if (Saved) {
        Saved = FlushFileBuffers(File) != FALSE;
    }

    if (!CloseHandle(File)) {
        Saved = false;
    }

    if (Saved) {
        Saved = MoveFileExW(TemporaryPath.c_str(), TargetPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    }

    if (!Saved) {
        std::filesystem::remove(TemporaryPath, Error);
    }

    return Saved;
}
