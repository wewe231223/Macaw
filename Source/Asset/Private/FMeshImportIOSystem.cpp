/**
 * @file FMeshImportIOSystem.cpp
 * @brief MTL 읽기에서 지원하지 않는 Prfl 행을 제거하는 Assimp 파일 입출력입니다.
 * @ingroup Asset
 */

#include "FMeshImportIOSystem.h"
#include "Core/FilePath.h"

#include <assimp/MemoryIOWrapper.h>
#include <cstring>
#include <memory>
#include <sstream>

namespace Asset {

    /**
     * @brief 기본 Assimp 파일 시스템으로 파일의 존재 여부를 검사합니다.
     */
    bool Asset::FMeshImportIOSystem::Exists(const char* File) const {
        return mDefaultIOSystem.Exists(File);
    }

    /**
     * @brief 기본 Assimp 파일 시스템의 경로 구분자를 반환합니다.
     */
    char Asset::FMeshImportIOSystem::getOsSeparator() const {
        return mDefaultIOSystem.getOsSeparator();
    }

    /**
     * @brief 파일을 열고 읽기용 MTL이면 Prfl 행을 제거한 메모리 스트림을 생성합니다.
     */
    Assimp::IOStream* Asset::FMeshImportIOSystem::Open(const char* File, const char* Mode) {
        if (File == nullptr || Mode == nullptr) {
            return nullptr;
        }

        if (Core::FilePath::ToLowerExtension(Core::FilePath::FromUtf8(File)) != L".mtl" || Mode[0] != 'r') {
            return mDefaultIOSystem.Open(File, Mode);
        }

        std::unique_ptr<Assimp::IOStream> Source{mDefaultIOSystem.Open(File, Mode)};

        if (Source == nullptr) {
            return nullptr;
        }

        Core::FString Text{};
        Text.resize(Source->FileSize());

        if (!Text.empty() && Source->Read(Text.data(), 1, Text.size()) != Text.size()) {
            return nullptr;
        }

        std::istringstream Input{Text};
        Core::FString Filtered{};
        Filtered.reserve(Text.size());

        Core::FString Line{};

        while (std::getline(Input, Line)) {
            std::istringstream Values{Line};
            Core::FString Keyword{};

            Values >> Keyword;

            if (Keyword != "Prfl") {
                Filtered += Line;
            }

            Filtered += '\n';
        }

        auto Bytes{std::make_unique<Core::Uint8[]>(Filtered.size())};

        if (!Filtered.empty()) {
            std::memcpy(Bytes.get(), Filtered.data(), Filtered.size());
        }

        auto* Stream{new Assimp::MemoryIOStream{Bytes.get(), Filtered.size(), true}};

        Bytes.release();

        return Stream;
    }

    /**
     * @brief Assimp 스트림을 삭제하고 스트림이 소유한 메모리를 해제합니다.
     */
    void Asset::FMeshImportIOSystem::Close(Assimp::IOStream* Stream) {
        delete Stream;
    }

    /**
     * @brief 기본 Assimp 파일 시스템으로 두 경로가 같은지 비교합니다.
     */
    bool Asset::FMeshImportIOSystem::ComparePaths(const char* First, const char* Second) const {
        return mDefaultIOSystem.ComparePaths(First, Second);
    }

}
