/**
 * @file FMaterialFile.cpp
 * @brief MTL 파싱, 프로젝트 참조 검증과 임시 파일 저장을 구현합니다.
 * @ingroup Engine
 */

#include "FMaterialFile.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"
#include "Engine/UMaterial.h"
#include "Core/FAtomicFileWriter.h"

#include <fstream>
#include <iomanip>
#include <limits>
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

bool FMaterialFile::Load(const std::filesystem::path& Path, FAssetManager& Assets, UMaterial& Material, FString& Error) {
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

bool FMaterialFile::Save(const std::filesystem::path& Path, const FAssetManager& Assets, const UMaterial& Material, FString& Error) {
    Error.clear();
    if (Path.extension() != L".mtl" || Material.GetName().empty() || Material.GetName().find_first_of("\r\n#") != FString::npos || Assets.ResolvePath(FilePath::FromUtf8(Material.GetShaderPath()), Error).empty()) {
        Error = "Invalid MTL material path or settings";
        return false;
    }
    FString TexturePath{};
    if (!Material.GetBaseColorTexturePath().empty()) {
        const auto Texture{Assets.ResolvePath(FilePath::FromUtf8(Material.GetBaseColorTexturePath()), Error)};
        if (Texture.empty()) {
            return false;
        }
        TexturePath = FilePath::ToGenericUtf8(Texture.lexically_relative(Path.parent_path()));
    }
    FString NormalPath{};
    if (!Material.GetNormalMapTexturePath().empty()) {
        const auto Normal{Assets.ResolvePath(FilePath::FromUtf8(Material.GetNormalMapTexturePath()), Error)};
        if (Normal.empty()) {
            return false;
        }
        NormalPath = FilePath::ToGenericUtf8(Normal.lexically_relative(Path.parent_path()));
    }
    FAtomicFileWriter Writer{};
    if (!Writer.Open(Path, Error)) {
        return false;
    }
    std::ostream& Output{Writer.GetStream()};
    Output.imbue(std::locale::classic());
    const FVector4& Color{Material.GetBaseColor()};
    Output << std::setprecision(std::numeric_limits<float>::max_digits10);
    Output << "newmtl " << Material.GetName() << "\r\nKd " << Color.x << ' ' << Color.y << ' ' << Color.z << "\r\nd " << Color.w << "\r\n";
    if (!TexturePath.empty()) {
        Output << "map_Kd " << TexturePath << "\r\n";
    }
    if (!NormalPath.empty()) {
        Output << "norm " << NormalPath << "\r\n";
    }
    Output << "Ns " << Material.GetShininess() << "\r\nspecular_strength " << Material.GetSpecularStrength() << "\r\nnormal_strength " << Material.GetNormalStrength() << "\r\n";
    Output << "shader " << Material.GetShaderPath() << "\r\ntwo_sided " << (Material.IsTwoSided() ? 1 : 0) << "\r\n";
    return Writer.Commit(Error);
}
