#include "FImGuiThemeCatalog.h"
#include "Core/FilePath.h"

#include <toml++/toml.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
    void ReadStyle(const toml::table& Table, ImGuiStyle& Style) {
        const std::pair<const char*, float ImGuiStyle::*> Floats[]{
            {"alpha", &ImGuiStyle::Alpha},
            {"disabledAlpha", &ImGuiStyle::DisabledAlpha},
            {"windowRounding", &ImGuiStyle::WindowRounding},
            {"windowBorderSize", &ImGuiStyle::WindowBorderSize},
            {"childRounding", &ImGuiStyle::ChildRounding},
            {"childBorderSize", &ImGuiStyle::ChildBorderSize},
            {"popupRounding", &ImGuiStyle::PopupRounding},
            {"popupBorderSize", &ImGuiStyle::PopupBorderSize},
            {"frameRounding", &ImGuiStyle::FrameRounding},
            {"frameBorderSize", &ImGuiStyle::FrameBorderSize},
            {"indentSpacing", &ImGuiStyle::IndentSpacing},
            {"columnsMinSpacing", &ImGuiStyle::ColumnsMinSpacing},
            {"scrollbarSize", &ImGuiStyle::ScrollbarSize},
            {"scrollbarRounding", &ImGuiStyle::ScrollbarRounding},
            {"grabMinSize", &ImGuiStyle::GrabMinSize},
            {"grabRounding", &ImGuiStyle::GrabRounding},
            {"tabRounding", &ImGuiStyle::TabRounding},
            {"tabBorderSize", &ImGuiStyle::TabBorderSize},
            {"tabMinWidthForCloseButton", &ImGuiStyle::TabCloseButtonMinWidthUnselected}
        };

        for (const auto& [Name, Member] : Floats) {
            if (Table.contains(Name)) {
                const auto Value{Table[Name].value<double>()};

                if (!Value || !std::isfinite(*Value) || *Value < 0.0 || *Value > 10000.0) {
                    throw std::runtime_error{FString{"Invalid style value: "} + Name};
                }

                Style.*Member = static_cast<float>(*Value);
            }
        }

        if (Style.Alpha < 0.2f || Style.Alpha > 1.0f || Style.DisabledAlpha > 1.0f) {
            throw std::runtime_error{"Theme alpha must keep the editor visible"};
        }

        const std::pair<const char*, ImVec2 ImGuiStyle::*> Vectors[]{
            {"windowPadding", &ImGuiStyle::WindowPadding},
            {"windowMinSize", &ImGuiStyle::WindowMinSize},
            {"windowTitleAlign", &ImGuiStyle::WindowTitleAlign},
            {"framePadding", &ImGuiStyle::FramePadding},
            {"itemSpacing", &ImGuiStyle::ItemSpacing},
            {"itemInnerSpacing", &ImGuiStyle::ItemInnerSpacing},
            {"cellPadding", &ImGuiStyle::CellPadding},
            {"buttonTextAlign", &ImGuiStyle::ButtonTextAlign},
            {"selectableTextAlign", &ImGuiStyle::SelectableTextAlign}
        };

        for (const auto& [Name, Member] : Vectors) {
            if (!Table.contains(Name)) {
                continue;
            }

            const toml::array* Values{Table[Name].as_array()};

            if (Values == nullptr || Values->size() != 2) {
                throw std::runtime_error{FString{"Invalid style vector: "} + Name};
            }

            const auto X{(*Values)[0].value<double>()};
            const auto Y{(*Values)[1].value<double>()};

            if (!X || !Y || !std::isfinite(*X) || !std::isfinite(*Y) || *X < 0.0 || *Y < 0.0 || *X > 10000.0 || *Y > 10000.0) {
                throw std::runtime_error{FString{"Invalid style vector: "} + Name};
            }

            Style.*Member = ImVec2{static_cast<float>(*X), static_cast<float>(*Y)};
        }

        Style.WindowMinSize.x = std::max(1.0f, Style.WindowMinSize.x);
        Style.WindowMinSize.y = std::max(1.0f, Style.WindowMinSize.y);

        for (const auto& [Name, Member] : {std::pair{"windowMenuButtonPosition", &ImGuiStyle::WindowMenuButtonPosition}, std::pair{"colorButtonPosition", &ImGuiStyle::ColorButtonPosition}}) {
            if (Table.contains(Name)) {
                const FString Value{Table[Name].value_or(FString{})};

                if (Value != "Left" && Value != "Right" && (Value != "None" || Member == &ImGuiStyle::ColorButtonPosition)) {
                    throw std::runtime_error{FString{"Invalid style direction: "} + Name};
                }

                Style.*Member = Value == "Left" ? ImGuiDir_Left : Value == "Right" ? ImGuiDir_Right : ImGuiDir_None;
            }
        }

        const toml::table* Colors{Table["colors"].as_table()};

        if (Colors == nullptr) {
            throw std::runtime_error{"The theme has no colors table"};
        }

        for (const auto& [Key, Node] : *Colors) {
            FString Name{Key.str()};

            if (Name == "TabActive") {
                Name = "TabSelected";
            } else if (Name == "TabUnfocused") {
                Name = "TabDimmed";
            } else if (Name == "TabUnfocusedActive") {
                Name = "TabDimmedSelected";
            } else if (Name == "NavHighlight") {
                Name = "NavCursor";
            }

            const auto Value{Node.value<FString>()};

            if (!Value || !Value->starts_with("rgba(") || !Value->ends_with(')')) {
                throw std::runtime_error{"Expected an ImThemes rgba color"};
            }

            FString Components{Value->substr(5, Value->size() - 6)};
            std::replace(Components.begin(), Components.end(), ',', ' ');
            std::istringstream Input{Components};
            std::array<float, 4> Channels{};

            for (float& Channel : Channels) {
                if (!(Input >> Channel) || !std::isfinite(Channel)) {
                    throw std::runtime_error{"Invalid ImThemes color channel"};
                }
            }

            Input >> std::ws;

            if (!Input.eof() || Channels[0] < 0.0f || Channels[0] > 255.0f || Channels[1] < 0.0f || Channels[1] > 255.0f || Channels[2] < 0.0f || Channels[2] > 255.0f || Channels[3] < 0.0f || Channels[3] > 1.0f) {
                throw std::runtime_error{"ImThemes color is out of range"};
            }

            for (int Index{}; Index < ImGuiCol_COUNT; ++Index) {
                if (Name == ImGui::GetStyleColorName(Index)) {
                    Style.Colors[Index] = ImVec4{Channels[0] / 255.0f, Channels[1] / 255.0f, Channels[2] / 255.0f, Channels[3]};
                    break;
                }
            }
        }

        Style.Colors[ImGuiCol_TabSelectedOverline] = Style.Colors[ImGuiCol_TabSelected];
        Style.Colors[ImGuiCol_TabDimmedSelectedOverline] = Style.Colors[ImGuiCol_TabDimmedSelected];
        Style.Colors[ImGuiCol_DockingPreview] = Style.Colors[ImGuiCol_HeaderActive];
        Style.Colors[ImGuiCol_DockingPreview].w = 0.7f;
        Style.Colors[ImGuiCol_DockingEmptyBg] = Style.Colors[ImGuiCol_WindowBg];
    }
}

FImGuiThemeCatalog::FImGuiThemeCatalog() {
    ImGuiStyle Style{};
    ImGui::StyleColorsDark(&Style);
    Style.WindowRounding = 3.0f;
    Style.FrameRounding = 3.0f;
    Style.GrabRounding = 3.0f;
    mNames.push_back("Macaw Dark");
    mAuthors.push_back("Macaw");
    mDescriptions.push_back("Default editor theme");
    mStyles.push_back(Style);
}

bool FImGuiThemeCatalog::Load(const std::filesystem::path& Path, FString& Error) {
    Error.clear();

    try {
        std::ifstream Input{Path, std::ios::binary};

        if (!Input) {
            throw std::runtime_error{"Cannot open the theme file"};
        }

        const toml::table Document{toml::parse(Input, FilePath::ToUtf8(Path))};
        const toml::array* Themes{Document["themes"].as_array()};
        TArray<const toml::table*> Tables{};

        if (Themes != nullptr) {
            for (const toml::node& Node : *Themes) {
                Tables.push_back(Node.as_table());
            }
        } else {
            Tables.push_back(&Document);
        }

        if (Tables.empty()) {
            throw std::runtime_error{"The file contains no ImThemes themes"};
        }

        FImGuiThemeCatalog Candidate{*this};

        for (const toml::table* Table : Tables) {
            if (Table == nullptr) {
                throw std::runtime_error{"Expected an ImThemes style table"};
            }

            const toml::table* StyleTable{Table->get_as<toml::table>("style")};

            if (StyleTable == nullptr && Table->contains("colors")) {
                StyleTable = Table;
            }

            if (StyleTable == nullptr) {
                throw std::runtime_error{"Expected an ImThemes style table"};
            }

            const FString Name{(*Table)["name"].value_or(FilePath::ToUtf8(Path.stem()))};

            if (Name.empty() || Name.find('\0') != FString::npos || Name == "Macaw Dark") {
                throw std::runtime_error{"Invalid or reserved theme name"};
            }

            ImGuiStyle Style{};
            ImGui::StyleColorsDark(&Style);
            ReadStyle(*StyleTable, Style);

            const auto Found{std::find(Candidate.mNames.begin(), Candidate.mNames.end(), Name)};
            const size_t Index{static_cast<size_t>(Found - Candidate.mNames.begin())};

            if (Found == Candidate.mNames.end()) {
                Candidate.mNames.push_back(Name);
                Candidate.mAuthors.emplace_back();
                Candidate.mDescriptions.emplace_back();
                Candidate.mStyles.emplace_back();
            }

            Candidate.mAuthors[Index] = (*Table)["author"].value_or(FString{});
            Candidate.mDescriptions[Index] = (*Table)["description"].value_or(FString{});
            Candidate.mStyles[Index] = Style;
        }

        *this = std::move(Candidate);
        return true;
    } catch (const std::exception& Exception) {
        Error = "Cannot load ImThemes '" + FilePath::ToUtf8(Path) + "': " + Exception.what();
        return false;
    }
}

Uint32 FImGuiThemeCatalog::GetCount() const {
    return static_cast<Uint32>(mNames.size());
}

Uint32 FImGuiThemeCatalog::Find(const FString& Name) const {
    const auto Found{std::find(mNames.begin(), mNames.end(), Name)};
    return Found == mNames.end() ? 0 : static_cast<Uint32>(Found - mNames.begin());
}

const FString& FImGuiThemeCatalog::GetName(Uint32 Index) const {
    return mNames.at(Index);
}

const FString& FImGuiThemeCatalog::GetAuthor(Uint32 Index) const {
    return mAuthors.at(Index);
}

const FString& FImGuiThemeCatalog::GetDescription(Uint32 Index) const {
    return mDescriptions.at(Index);
}

const ImGuiStyle& FImGuiThemeCatalog::GetStyle(Uint32 Index) const {
    return mStyles.at(Index);
}
