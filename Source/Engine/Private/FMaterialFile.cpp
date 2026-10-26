#include "FMaterialFile.h"
#include "Engine/FAssetManager.h"
#include "Engine/UMaterial.h"
#include "Core/Guid.h"

#include <Windows.h>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace {
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

    std::filesystem::path FromText(const FString& Text) {
        return std::filesystem::path{std::u8string_view{reinterpret_cast<const char8_t*>(Text.data()), Text.size()}};
    }

    FString ToText(const std::filesystem::path& Path) {
        const auto Text{Path.generic_u8string()};
        return {reinterpret_cast<const char*>(Text.data()), Text.size()};
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
        } else if (Keyword == "map_Kd") {
            const FString Reference{ReadRemainder(Values)};
            if (Reference.empty() || Reference.front() == '-') {
                Error = "map_Kd requires a texture path without map options";
                return false;
            }
            const FString Relative{Assets.GetRelativePath(Path.parent_path() / FromText(Reference), Error)};
            Valid = !Relative.empty() && Material.SetBaseColorTexturePath(Relative);
        } else if (Keyword == "shader") {
            const FString Shader{ReadRemainder(Values)};
            Valid = Material.SetShaderPath(Shader) && !Assets.ResolvePath(FromText(Shader), Error).empty();
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
    if (Path.extension() != L".mtl" || Material.GetName().empty() || Material.GetName().find_first_of("\r\n#") != FString::npos || Assets.ResolvePath(FromText(Material.GetShaderPath()), Error).empty()) {
        Error = "Invalid MTL material path or settings";
        return false;
    }
    std::error_code Code{};
    std::filesystem::create_directories(Path.parent_path(), Code);
    if (Code) {
        Error = "Cannot create the material directory: " + Code.message();
        return false;
    }
    FString TexturePath{};
    if (!Material.GetBaseColorTexturePath().empty()) {
        const auto Texture{Assets.ResolvePath(FromText(Material.GetBaseColorTexturePath()), Error)};
        if (Texture.empty()) {
            return false;
        }
        TexturePath = ToText(Texture.lexically_relative(Path.parent_path()));
    }
    std::filesystem::path Temporary{Path};
    Temporary += L"." + FromText(FGuid{}.ToString()).wstring() + L".tmp";
    {
        std::ofstream Output{Temporary, std::ios::binary | std::ios::trunc};
        Output.imbue(std::locale::classic());
        const FVector4& Color{Material.GetBaseColor()};
        Output << std::setprecision(std::numeric_limits<float>::max_digits10);
        Output << "newmtl " << Material.GetName() << "\r\nKd " << Color.x << ' ' << Color.y << ' ' << Color.z << "\r\nd " << Color.w << "\r\n";
        if (!TexturePath.empty()) {
            Output << "map_Kd " << TexturePath << "\r\n";
        }
        Output << "shader " << Material.GetShaderPath() << "\r\ntwo_sided " << (Material.IsTwoSided() ? 1 : 0) << "\r\n";
        Output.flush();
        Output.close();
        if (!Output) {
            std::filesystem::remove(Temporary, Code);
            Error = "Cannot write the MTL material";
            return false;
        }
    }
    if (!MoveFileExW(Temporary.c_str(), Path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(Temporary, Code);
        Error = "Cannot replace the MTL material";
        return false;
    }
    return true;
}
