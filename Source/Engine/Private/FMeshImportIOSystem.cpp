/**
 * @file FMeshImportIOSystem.cpp
 * @brief MTL 읽기에서 지원하지 않는 Prfl 행을 제거하는 Assimp 파일 입출력입니다.
 * @ingroup Engine
 */

#include "FMeshImportIOSystem.h"
#include "Core/FilePath.h"

#include <assimp/MemoryIOWrapper.h>
#include <cstring>
#include <cwctype>
#include <memory>
#include <sstream>

bool FMeshImportIOSystem::Exists(const char* File) const {
    return mDefaultIOSystem.Exists(File);
}

char FMeshImportIOSystem::getOsSeparator() const {
    return mDefaultIOSystem.getOsSeparator();
}

Assimp::IOStream* FMeshImportIOSystem::Open(const char* File, const char* Mode) {
    if (File == nullptr || Mode == nullptr) {
        return nullptr;
    }
    std::wstring Extension{FilePath::FromUtf8(File).extension().wstring()};
    for (wchar_t& Character : Extension) {
        Character = static_cast<wchar_t>(std::towlower(Character));
    }
    if (Extension != L".mtl" || Mode[0] != 'r') {
        return mDefaultIOSystem.Open(File, Mode);
    }
    std::unique_ptr<Assimp::IOStream> Source{mDefaultIOSystem.Open(File, Mode)};
    if (Source == nullptr) {
        return nullptr;
    }
    FString Text{};
    Text.resize(Source->FileSize());
    if (!Text.empty() && Source->Read(Text.data(), 1, Text.size()) != Text.size()) {
        return nullptr;
    }
    std::istringstream Input{Text};
    FString Filtered{};
    Filtered.reserve(Text.size());
    FString Line{};
    while (std::getline(Input, Line)) {
        std::istringstream Values{Line};
        FString Keyword{};
        Values >> Keyword;
        if (Keyword != "Prfl") {
            Filtered += Line;
        }
        Filtered += '\n';
    }
    auto Bytes{std::make_unique<Uint8[]>(Filtered.size())};
    if (!Filtered.empty()) {
        std::memcpy(Bytes.get(), Filtered.data(), Filtered.size());
    }
    auto* Stream{new Assimp::MemoryIOStream{Bytes.get(), Filtered.size(), true}};
    Bytes.release();
    return Stream;
}

void FMeshImportIOSystem::Close(Assimp::IOStream* Stream) {
    delete Stream;
}

bool FMeshImportIOSystem::ComparePaths(const char* First, const char* Second) const {
    return mDefaultIOSystem.ComparePaths(First, Second);
}
