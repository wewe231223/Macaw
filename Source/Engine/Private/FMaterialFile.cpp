/**
 * @file FMaterialFile.cpp
 * @brief 재질 직렬화, 기존 MTL 변환과 프로젝트 참조 검증을 구현합니다.
 * @ingroup Engine
 */

#include "FMaterialFile.h"
#include "FMaterialAssetData.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"
#include "Engine/UMaterial.h"
#include "Engine/FAssetFile.h"
#include "Serialization/MemoryArchive.h"

#include <fstream>
#include <locale>
#include <sstream>

namespace Engine {

    /**
     * @brief MTL 한 줄의 문자열 값을 해석하는 내부 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 남은 행의 공백과 선택적인 인용부호를 제거합니다.
         * @param[in,out] Input 현재 MTL 행의 입력 스트림입니다.
         * @return 남아 있는 필드 문자열입니다.
         */
        Core::FString ReadRemainder(std::istream& Input) {
            Core::FString Value{};
            std::getline(Input >> std::ws, Value);

            const auto End{Value.find_last_not_of(" \t\r")};

            if (End == Core::FString::npos) {
                return {};
            }

            Value.resize(End + 1);

            if (Value.size() >= 2 && Value.front() == '"' && Value.back() == '"') {
                return Value.substr(1, Value.size() - 2);
            }

            return Value;
        }
    }

    bool Engine::FMaterialFile::LoadLegacy(const std::filesystem::path& Path, Engine::FAssetManager& Assets, Engine::UMaterial& Material, Core::FString& Error) {
        Error.clear();

        std::ifstream Input{Path};

        if (!Input || Path.extension() != L".mtl") {
            Error = "Cannot open the MTL material";
            return false;
        }

        Core::FString Line{};
        bool Named{};
        Core::Uint32 LineNumber{};

        while (std::getline(Input, Line)) {
            ++LineNumber;

            if (LineNumber == 1 && Line.starts_with("\xEF\xBB\xBF")) {
                Line.erase(0, 3);
            }

            Line.resize(Line.find('#') == Core::FString::npos ? Line.size() : Line.find('#'));

            std::istringstream Values{Line};
            Values.imbue(std::locale::classic());

            Core::FString Keyword{};

            if (!(Values >> Keyword)) {
                continue;
            }

            bool Valid{true};

            if (Keyword == "newmtl") {
                const Core::FString Name{ReadRemainder(Values)};

                if (Named) {
                    Error = "Use one material per .mtl asset";
                    return false;
                }

                Valid = Material.SetName(Name);
                Named = Valid;
            } else if (!Named) {
                Valid = false;
            } else if (Keyword == "Kd") {
                Core::FVector4 Color{Material.GetBaseColor()};
                Valid = static_cast<bool>(Values >> Color.x >> Color.y >> Color.z) && Material.SetBaseColor(Color);
            } else if (Keyword == "d" || Keyword == "Tr") {
                float Alpha{};
                Core::FVector4 Color{Material.GetBaseColor()};
                Valid = static_cast<bool>(Values >> Alpha);
                Color.w = Keyword == "Tr" ? 1.0f - Alpha : Alpha;
                Valid = Valid && Material.SetBaseColor(Color);
            } else if (Keyword == "Ns" || Keyword == "specular_strength" || Keyword == "normal_strength") {
                float Value{};
                Valid = static_cast<bool>(Values >> Value) && (Keyword == "Ns" ? Material.SetShininess(Value) : Keyword == "specular_strength" ? Material.SetSpecularStrength(Value) : Material.SetNormalStrength(Value));
            } else if (Keyword == "norm" || Keyword == "map_Bump" || Keyword == "map_bump" || Keyword == "bump") {
                const Core::FString Reference{ReadRemainder(Values)};
                const Core::FString Relative{Reference.empty() || Reference.front() == '-' ? Core::FString{} : Assets.GetRelativePath(Path.parent_path() / Core::FilePath::FromUtf8(Reference), Error)};
                Valid = !Relative.empty() && Material.SetNormalMapTexturePath(Relative);
            } else if (Keyword == "map_Kd") {
                const Core::FString Reference{ReadRemainder(Values)};

                if (Reference.empty() || Reference.front() == '-') {
                    Error = "map_Kd requires a texture path without map options";
                    return false;
                }

                const Core::FString Relative{Assets.GetRelativePath(Path.parent_path() / Core::FilePath::FromUtf8(Reference), Error)};
                Valid = !Relative.empty() && Material.SetBaseColorTexturePath(Relative);
            } else if (Keyword == "shader") {
                const Core::FString Shader{ReadRemainder(Values)};
                Valid = Material.SetShaderPath(Shader) && !Assets.ResolvePath(Core::FilePath::FromUtf8(Shader), Error).empty();
            } else if (Keyword == "two_sided") {
                Core::Int32 TwoSided{};
                Valid = static_cast<bool>(Values >> TwoSided) && (TwoSided == 0 || TwoSided == 1) && Material.SetTwoSided(TwoSided != 0);
            }

            if (!Valid) {
                Error = "Invalid MTL value at line " + std::to_string(LineNumber) + (Error.empty() ? Core::FString{} : ": " + Error);
                return false;
            }
        }

        if (!Named || !Input.eof()) {
            Error = "Invalid or unreadable MTL material";
            return false;
        }

        return Material.ResolveTexture(Assets, Error);
    }

    bool Engine::FMaterialFile::Load(const std::filesystem::path& Path, Engine::FAssetManager& Assets, Engine::UMaterial& Material, Core::FString& Error) {
        Core::TArray<Core::Uint8> Payload{};

        if (!Engine::FAssetFile::Load(Path, Engine::EAssetType::Material, Payload, Error)) {
            return false;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Engine::FMaterialAssetData Data{};
        Data.Serialize(Archive);

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
            Error = Archive.HasError() ? Archive.GetError() : "Unexpected material asset data";
            return false;
        }

        return Data.ValidateReferences(Assets, Error) && Data.ApplyTo(Material, Error) && Material.ResolveTexture(Assets, Error);
    }

    bool Engine::FMaterialFile::Save(const std::filesystem::path& Path, const Engine::FAssetManager& Assets, const Engine::UMaterial& Material, Core::FString& Error) {
        Error.clear();

        Engine::FMaterialAssetData Data{Material};

        if (!Data.ValidateReferences(Assets, Error)) {
            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Data.Serialize(Archive);

        if (Archive.HasError()) {
            Error = Archive.GetError();
            return false;
        }

        return Engine::FAssetFile::Save(Path, Engine::EAssetType::Material, Payload, Error);
    }

}
