/**
 * @file FProjectTemplatePicker.cpp
 * @brief 프로젝트 템플릿 목록과 미리보기 이미지를 읽고 선택 UI를 제공합니다.
 * @ingroup Editor
 */

#include "Editor/FProjectTemplatePicker.h"
#include "Asset/UTexture2D.h"
#include "Core/FilePath.h"
#include "Serialization/JsonArchive.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace Editor {

    FProjectTemplatePicker::FProjectTemplatePicker() = default;

    FProjectTemplatePicker::~FProjectTemplatePicker() = default;

    bool FProjectTemplatePicker::Load(const std::filesystem::path& Directory, const std::filesystem::path& SelectedDirectory, Core::FString& Error) {
        const auto Preferred{SelectedDirectory.lexically_normal()};
        mTemplates.clear();
        mSelectedIndex = 0;
        Error.clear();
        std::error_code Code{};
        std::filesystem::directory_iterator Entry{Directory, Code};

        if (Code) {
            Error = "Cannot find the project templates: " + Code.message();

            return false;
        }

        const std::filesystem::directory_iterator End{};

        while (Entry != End && !Code) {
            const auto Path{Entry->path()};

            if (Entry->is_directory(Code) && std::filesystem::exists(Path / "Template.mproject", Code) && !LoadTemplate(Path, Error)) {
                return false;
            }

            if (Code) {
                break;
            }

            Entry.increment(Code);
        }

        if (Code || mTemplates.empty()) {
            Error = Code ? "Cannot read the project templates: " + Code.message() : "No project templates are installed";

            return false;
        }

        std::sort(mTemplates.begin(), mTemplates.end(), [](const auto& A, const auto& B) {
            if ((A.mId == "Default") != (B.mId == "Default")) {
                return A.mId == "Default";
            }

            return A.mName < B.mName;
        });

        for (std::size_t Index{}; Index < mTemplates.size(); ++Index) {
            if (mTemplates[Index].mDirectory.lexically_normal() == Preferred) {
                mSelectedIndex = Index;
            }
        }

        return true;
    }

    bool FProjectTemplatePicker::LoadTemplate(const std::filesystem::path& Directory, Core::FString& Error) {
        FTemplateInfo Template{};
        Template.mDirectory = Directory;
        Template.mId = Core::FilePath::ToUtf8(Directory.filename());
        Template.mName = Template.mId;
        Template.mDescription = "Create a project from this template.";
        const auto Manifest{Directory / "Template.json"};
        std::error_code Code{};

        if (std::filesystem::is_regular_file(Manifest, Code)) {
            Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};
            Core::FString Preview{};

            if (!Archive.Load(Manifest)) {
                Error = Archive.GetError();

                return false;
            }

            Archive.Serialize("Id", Template.mId);
            Archive.Serialize("Name", Template.mName);
            Archive.Serialize("Description", Template.mDescription);

            if (Archive.HasField("RequiresScripts")) {
                Archive.Serialize("RequiresScripts", Template.mRequiresScripts);
            }

            if (Archive.HasField("Preview")) {
                Archive.Serialize("Preview", Preview);
            }

            const std::filesystem::path PreviewPath{Core::FilePath::FromUtf8(Preview)};
            const bool InvalidPreview{PreviewPath.has_root_path() || std::any_of(PreviewPath.begin(), PreviewPath.end(), [](const auto& Part) {
                return Part == "..";
            })};

            if (Archive.HasError() || Template.mId.empty() || Template.mName.empty() || InvalidPreview) {
                Error = Archive.HasError() ? Archive.GetError() : "Invalid project template metadata";

                return false;
            }

            if (!Preview.empty()) {
                Template.mPreviewPath = Directory / PreviewPath;
            }
        }

        if (std::any_of(mTemplates.begin(), mTemplates.end(), [&Template](const auto& Existing) {
            return Existing.mId == Template.mId;
        })) {
            Error = "Duplicate project template identifier: " + Template.mId;

            return false;
        }

        mTemplates.push_back(std::move(Template));

        return true;
    }

    bool FProjectTemplatePicker::SelectTemplate(const Core::FString& Id) {
        const auto Found{std::find_if(mTemplates.begin(), mTemplates.end(), [&Id](const auto& Template) {
            return Template.mId == Id;
        })};

        if (Found == mTemplates.end()) {
            return false;
        }

        mSelectedIndex = static_cast<std::size_t>(Found - mTemplates.begin());

        return true;
    }

    const std::filesystem::path& FProjectTemplatePicker::GetSelectedDirectory() const {
        static const std::filesystem::path Empty{};

        return mSelectedIndex < mTemplates.size() ? mTemplates[mSelectedIndex].mDirectory : Empty;
    }

    Core::Uint32 FProjectTemplatePicker::GetTemplateCount() const {
        return static_cast<Core::Uint32>(mTemplates.size());
    }

    void FProjectTemplatePicker::LoadPreview(FTemplateInfo& Template, void* Device) {
        if (Device == nullptr || Template.mPreviewAttempted || Template.mPreviewPath.empty()) {
            return;
        }

        Template.mPreviewAttempted = true;
        auto Texture{std::make_unique<Asset::UTexture2D>()};

        if (Texture->LoadFromFile(Template.mPreviewPath, false) && Texture->CreateResources(static_cast<ID3D11Device*>(Device))) {
            Template.mPreview = std::move(Texture);
        }
    }

    bool FProjectTemplatePicker::Draw(void* Device) {
        if (mTemplates.empty()) {
            return false;
        }

        const float FontSize{ImGui::GetFontSize()};
        const float Scale{FontSize / 18.0f};
        const float Available{ImGui::GetContentRegionAvail().x};
        const int Columns{std::min(static_cast<int>(mTemplates.size()), std::max(1, static_cast<int>(Available / (300.0f * Scale))))};
        bool Changed{};
        ImGui::TextUnformatted("Choose a template");

        if (!ImGui::BeginTable("ProjectTemplates", Columns, ImGuiTableFlags_SizingStretchSame)) {
            return false;
        }

        for (std::size_t Index{}; Index < mTemplates.size(); ++Index) {
            auto& Template{mTemplates[Index]};
            ImGui::TableNextColumn();
            ImGui::PushID(Template.mId.c_str());
            const float Width{ImGui::GetContentRegionAvail().x};
            const float Height{270.0f * Scale};

            if (ImGui::Selectable("##Template", mSelectedIndex == Index, ImGuiSelectableFlags_None, {Width, Height})) {
                mSelectedIndex = Index;
                Changed = true;
            }

            ImDrawList* Draw{ImGui::GetWindowDrawList()};
            const ImVec2 ItemMin{ImGui::GetItemRectMin()};
            const ImVec2 ItemMax{ImGui::GetItemRectMax()};
            const ImVec2 Min{std::max(ItemMin.x, Draw->GetClipRectMin().x + 1.0f), ItemMin.y};
            const ImVec2 Max{std::min(ItemMax.x, Draw->GetClipRectMax().x - 1.0f), ItemMax.y};
            const float DrawWidth{Max.x - Min.x};
            const float Padding{14.0f * Scale};
            const float PreviewHeight{std::min(145.0f * Scale, (DrawWidth - Padding * 2.0f) * 9.0f / 16.0f)};
            const float PreviewWidth{PreviewHeight * 16.0f / 9.0f};
            const ImVec2 PreviewMin{Min.x + (DrawWidth - PreviewWidth) * 0.5f, Min.y + Padding};
            const ImVec2 PreviewMax{PreviewMin.x + PreviewWidth, PreviewMin.y + PreviewHeight};
            Draw->AddRect(Min, Max, ImGui::GetColorU32(mSelectedIndex == Index ? ImVec4{0.2f, 0.65f, 0.95f, 1.0f} : ImVec4{0.23f, 0.29f, 0.37f, 1.0f}), 6.0f * Scale, 0, mSelectedIndex == Index ? 2.0f : 1.0f);
            LoadPreview(Template, Device);

            if (Template.mPreview != nullptr) {
                Draw->AddImage(reinterpret_cast<ImTextureID>(Template.mPreview->GetShaderResourceView()), PreviewMin, PreviewMax);
            } else {
                Draw->AddRectFilled(PreviewMin, PreviewMax, IM_COL32(19, 30, 45, 255), 4.0f * Scale);
                const ImVec2 Center{(PreviewMin.x + PreviewMax.x) * 0.5f, (PreviewMin.y + PreviewMax.y) * 0.5f};
                const float W{39.0f * Scale};
                const float H{24.0f * Scale};
                Draw->AddQuadFilled({Center.x, Center.y - H}, {Center.x + W, Center.y - H * 0.3f}, Center, {Center.x - W, Center.y - H * 0.3f}, IM_COL32(139, 185, 218, 255));
                Draw->AddQuadFilled({Center.x - W, Center.y - H * 0.3f}, Center, {Center.x, Center.y + H * 1.5f}, {Center.x - W, Center.y + H * 1.2f}, IM_COL32(71, 116, 153, 255));
                Draw->AddQuadFilled(Center, {Center.x + W, Center.y - H * 0.3f}, {Center.x + W, Center.y + H * 1.2f}, {Center.x, Center.y + H * 1.5f}, IM_COL32(44, 77, 109, 255));
            }

            const float TextY{PreviewMax.y + Padding};
            Draw->AddText(ImGui::GetFont(), FontSize * 1.15f, {Min.x + Padding, TextY}, ImGui::GetColorU32(ImGuiCol_Text), Template.mName.c_str());
            Draw->AddText(ImGui::GetFont(), FontSize * 0.9f, {Min.x + Padding, TextY + FontSize * 1.7f}, ImGui::GetColorU32(ImGuiCol_TextDisabled), Template.mDescription.c_str(), nullptr, DrawWidth - Padding * 2.0f);

            if (Template.mRequiresScripts) {
                Draw->AddText(ImGui::GetFont(), FontSize * 0.85f, {Min.x + Padding, Max.y - FontSize * 1.8f}, ImGui::GetColorU32(ImVec4{0.45f, 0.78f, 0.95f, 1.0f}), "C# gameplay / .NET 10 SDK");
            }

            ImGui::PopID();
        }

        ImGui::EndTable();

        return Changed;
    }

}
