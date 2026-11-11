#pragma once

#include "Core/Types.h"

#include <filesystem>
#include <memory>

namespace Asset {
    class UTexture2D;
}

namespace Editor {

    class FProjectTemplatePicker {
    private:
        struct FTemplateInfo {
            Core::FString mId{};
            Core::FString mName{};
            Core::FString mDescription{};
            std::filesystem::path mDirectory{};
            std::filesystem::path mPreviewPath{};
            std::unique_ptr<Asset::UTexture2D> mPreview{};
            bool mRequiresScripts{};
            bool mPreviewAttempted{};
        };

    public:
        FProjectTemplatePicker();
        ~FProjectTemplatePicker();

        FProjectTemplatePicker(const Editor::FProjectTemplatePicker&) = delete;
        Editor::FProjectTemplatePicker& operator=(const Editor::FProjectTemplatePicker&) = delete;

    public:
        bool Load(const std::filesystem::path& Directory, const std::filesystem::path& SelectedDirectory, Core::FString& Error);
        bool SelectTemplate(const Core::FString& Id);
        const std::filesystem::path& GetSelectedDirectory() const;
        Core::Uint32 GetTemplateCount() const;
        bool Draw(void* Device = nullptr);

    private:
        bool LoadTemplate(const std::filesystem::path& Directory, Core::FString& Error);
        void LoadPreview(FTemplateInfo& Template, void* Device);

    private:
        Core::TArray<FTemplateInfo> mTemplates{};
        std::size_t mSelectedIndex{};
    };

}
