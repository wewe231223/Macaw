/**
 * @file FImGuiThemeCatalog.cpp
 * @brief 기본 테마와 ImThemes TOML에서 읽은 이름·작성자·설명·ImGui 스타일을 관리합니다.
 * @ingroup Editor
 */

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

namespace Editor {

    /**
     * @brief 이 구현 파일에서만 사용하는 검증·변환·설정 보조 함수를 제공합니다.
     */
    namespace {
        /**
         * @brief ImThemes 스타일 표의 수치·방향·RGBA 색상을 검증하여 ImGui 스타일에 반영합니다.
         * @param[in] Table 검증할 ImThemes 스타일의 TOML 표입니다.
         * @param[in,out] Style 읽거나 갱신할 배율 적용 전 ImGui 스타일입니다.
         * @note 잘못된 수치·방향·색상 형식이면 std::runtime_error를 발생시키며 호출자가 오류를 처리합니다.
         */
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
                        throw std::runtime_error{Core::FString{"Invalid style value: "} + Name};
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
                    throw std::runtime_error{Core::FString{"Invalid style vector: "} + Name};
                }

                const auto X{(*Values)[0].value<double>()};
                const auto Y{(*Values)[1].value<double>()};

                if (!X || !Y || !std::isfinite(*X) || !std::isfinite(*Y) || *X < 0.0 || *Y < 0.0 || *X > 10000.0 || *Y > 10000.0) {
                    throw std::runtime_error{Core::FString{"Invalid style vector: "} + Name};
                }

                Style.*Member = ImVec2{static_cast<float>(*X), static_cast<float>(*Y)};
            }

            Style.WindowMinSize.x = std::max(1.0f, Style.WindowMinSize.x);
            Style.WindowMinSize.y = std::max(1.0f, Style.WindowMinSize.y);

            for (const auto& [Name, Member] : {std::pair{"windowMenuButtonPosition", &ImGuiStyle::WindowMenuButtonPosition}, std::pair{"colorButtonPosition", &ImGuiStyle::ColorButtonPosition}}) {
                if (Table.contains(Name)) {
                    const Core::FString Value{Table[Name].value_or(Core::FString{})};

                    if (Value != "Left" && Value != "Right" && (Value != "None" || Member == &ImGuiStyle::ColorButtonPosition)) {
                        throw std::runtime_error{Core::FString{"Invalid style direction: "} + Name};
                    }

                    Style.*Member = Value == "Left" ? ImGuiDir_Left : Value == "Right" ? ImGuiDir_Right : ImGuiDir_None;
                }
            }

            const toml::table* Colors{Table["colors"].as_table()};

            if (Colors == nullptr) {
                throw std::runtime_error{"The theme has no colors table"};
            }

            for (const auto& [Key, Node] : *Colors) {
                Core::FString Name{Key.str()};

                if (Name == "TabActive") {
                    Name = "TabSelected";
                } else if (Name == "TabUnfocused") {
                    Name = "TabDimmed";
                } else if (Name == "TabUnfocusedActive") {
                    Name = "TabDimmedSelected";
                } else if (Name == "NavHighlight") {
                    Name = "NavCursor";
                }

                const auto Value{Node.value<Core::FString>()};

                if (!Value || !Value->starts_with("rgba(") || !Value->ends_with(')')) {
                    throw std::runtime_error{"Expected an ImThemes rgba color"};
                }

                Core::FString Components{Value->substr(5, Value->size() - 6)};
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

    Editor::FImGuiThemeCatalog::FImGuiThemeCatalog() {
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

    bool Editor::FImGuiThemeCatalog::Load(const std::filesystem::path& Path, Core::FString& Error) {
        Error.clear();

        try {
            std::ifstream Input{Path, std::ios::binary};

            if (!Input) {
                throw std::runtime_error{"Cannot open the theme file"};
            }

            const toml::table Document{toml::parse(Input, Core::FilePath::ToUtf8(Path))};
            const toml::array* Themes{Document["themes"].as_array()};
            Core::TArray<const toml::table*> Tables{};

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

            Editor::FImGuiThemeCatalog Candidate{*this};

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

                const Core::FString Name{(*Table)["name"].value_or(Core::FilePath::ToUtf8(Path.stem()))};

                if (Name.empty() || Name.find('\0') != Core::FString::npos || Name == "Macaw Dark") {
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

                Candidate.mAuthors[Index] = (*Table)["author"].value_or(Core::FString{});
                Candidate.mDescriptions[Index] = (*Table)["description"].value_or(Core::FString{});
                Candidate.mStyles[Index] = Style;
            }

            *this = std::move(Candidate);
            return true;
        } catch (const std::exception& Exception) {
            Error = "Cannot load ImThemes '" + Core::FilePath::ToUtf8(Path) + "': " + Exception.what();
            return false;
        }
    }

    Core::Uint32 Editor::FImGuiThemeCatalog::GetCount() const {
        return static_cast<Core::Uint32>(mNames.size());
    }

    Core::Uint32 Editor::FImGuiThemeCatalog::Find(const Core::FString& Name) const {
        const auto Found{std::find(mNames.begin(), mNames.end(), Name)};
        return Found == mNames.end() ? 0 : static_cast<Core::Uint32>(Found - mNames.begin());
    }

    const Core::FString& Editor::FImGuiThemeCatalog::GetName(Core::Uint32 Index) const {
        return mNames.at(Index);
    }

    const Core::FString& Editor::FImGuiThemeCatalog::GetAuthor(Core::Uint32 Index) const {
        return mAuthors.at(Index);
    }

    const Core::FString& Editor::FImGuiThemeCatalog::GetDescription(Core::Uint32 Index) const {
        return mDescriptions.at(Index);
    }

    const ImGuiStyle& Editor::FImGuiThemeCatalog::GetStyle(Core::Uint32 Index) const {
        return mStyles.at(Index);
    }

}
