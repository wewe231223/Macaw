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

/**
 * @brief MTL 한 줄의 문자열 값을 해석하는 내부 보조 함수입니다.
 */
namespace {
    /**
     * @brief 남은 행의 공백과 선택적인 인용부호를 제거합니다.
     * @param[in,out] Input 현재 MTL 행의 입력 스트림입니다.
     * @return 남아 있는 필드 문자열입니다.
     */
    FString ReadRemainder(std::istream& Input) {
        FString Value{};
        std::getline(Input >> std::ws, Value);

        const auto End{Value.find_last_not_of(" \t\r")};

        if (End == FString::npos) {
            return {};
        }

        Value.resize(End + 1);

        if (Value.size() >= 2 && Value.front() == '"' && Value.back() == '"') {
            return Value.substr(1, Value.size() - 2);
        }

        return Value;
    }
}

bool FMaterialFile::LoadLegacy(const std::filesystem::path& Path, FAssetManager& Assets, UMaterial& Material, FString& Error) {
    Error.clear();

    std::ifstream Input{Path};

    if (!Input || Path.extension() != L".mtl") {
        Error = "Cannot open the MTL material";
        return false;
    }

    FString Line{};
    bool Named{};
    Uint32 LineNumber{};

    while (std::getline(Input, Line)) {
        ++LineNumber;

        if (LineNumber == 1 && Line.starts_with("\xEF\xBB\xBF")) {
            Line.erase(0, 3);
        }

        Line.resize(Line.find('#') == FString::npos ? Line.size() : Line.find('#'));

        std::istringstream Values{Line};
        Values.imbue(std::locale::classic());

        FString Keyword{};

        if (!(Values >> Keyword)) {
            continue;
        }

        bool Valid{true};

        if (Keyword == "newmtl") {
            const FString Name{ReadRemainder(Values)};

            if (Named) {
                Error = "Use one material per .mtl asset";
                return false;
            }

            Valid = Material.SetName(Name);
            Named = Valid;
        } else if (!Named) {
            Valid = false;
        } else if (Keyword == "Kd") {
            FVector4 Color{Material.GetBaseColor()};
            Valid = static_cast<bool>(Values >> Color.x >> Color.y >> Color.z) && Material.SetBaseColor(Color);
        } else if (Keyword == "d" || Keyword == "Tr") {
            float Alpha{};
            FVector4 Color{Material.GetBaseColor()};
            Valid = static_cast<bool>(Values >> Alpha);
            Color.w = Keyword == "Tr" ? 1.0f - Alpha : Alpha;
            Valid = Valid && Material.SetBaseColor(Color);
        } else if (Keyword == "Ns" || Keyword == "specular_strength" || Keyword == "normal_strength") {
            float Value{};
            Valid = static_cast<bool>(Values >> Value) && (Keyword == "Ns" ? Material.SetShininess(Value) : Keyword == "specular_strength" ? Material.SetSpecularStrength(Value) : Material.SetNormalStrength(Value));
        } else if (Keyword == "norm" || Keyword == "map_Bump" || Keyword == "map_bump" || Keyword == "bump") {
            const FString Reference{ReadRemainder(Values)};
            const FString Relative{Reference.empty() || Reference.front() == '-' ? FString{} : Assets.GetRelativePath(Path.parent_path() / FilePath::FromUtf8(Reference), Error)};
            Valid = !Relative.empty() && Material.SetNormalMapTexturePath(Relative);
        } else if (Keyword == "map_Kd") {
            const FString Reference{ReadRemainder(Values)};

            if (Reference.empty() || Reference.front() == '-') {
                Error = "map_Kd requires a texture path without map options";
                return false;
            }

            const FString Relative{Assets.GetRelativePath(Path.parent_path() / FilePath::FromUtf8(Reference), Error)};
            Valid = !Relative.empty() && Material.SetBaseColorTexturePath(Relative);
        } else if (Keyword == "shader") {
            const FString Shader{ReadRemainder(Values)};
            Valid = Material.SetShaderPath(Shader) && !Assets.ResolvePath(FilePath::FromUtf8(Shader), Error).empty();
        } else if (Keyword == "two_sided") {
            Int32 TwoSided{};
            Valid = static_cast<bool>(Values >> TwoSided) && (TwoSided == 0 || TwoSided == 1) && Material.SetTwoSided(TwoSided != 0);
        }

        if (!Valid) {
            Error = "Invalid MTL value at line " + std::to_string(LineNumber) + (Error.empty() ? FString{} : ": " + Error);
            return false;
        }
    }

    if (!Named || !Input.eof()) {
        Error = "Invalid or unreadable MTL material";
        return false;
    }

    return Material.ResolveTexture(Assets, Error);
}

bool FMaterialFile::Load(const std::filesystem::path& Path, FAssetManager& Assets, UMaterial& Material, FString& Error) {
    TArray<Uint8> Payload{};

    if (!FAssetFile::Load(Path, EAssetType::Material, Payload, Error)) {
        return false;
    }

    FMemoryArchive Archive{static_cast<const TArray<Uint8>&>(Payload)};
    FMaterialAssetData Data{};
    Data.Serialize(Archive);

    if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
        Error = Archive.HasError() ? Archive.GetError() : "Unexpected material asset data";
        return false;
    }

    return Data.ValidateReferences(Assets, Error) && Data.ApplyTo(Material, Error) && Material.ResolveTexture(Assets, Error);
}

bool FMaterialFile::Save(const std::filesystem::path& Path, const FAssetManager& Assets, const UMaterial& Material, FString& Error) {
    Error.clear();

    FMaterialAssetData Data{Material};

    if (!Data.ValidateReferences(Assets, Error)) {
        return false;
    }

    TArray<Uint8> Payload{};
    FMemoryArchive Archive{Payload};
    Data.Serialize(Archive);

    if (Archive.HasError()) {
        Error = Archive.GetError();
        return false;
    }

    return FAssetFile::Save(Path, EAssetType::Material, Payload, Error);
}
