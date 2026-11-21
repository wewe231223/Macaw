/**
 * @file FContentBrowserWidget.cpp
 * @brief 콘텐츠 탐색과 에셋 생성·가져오기·이름 변경·삭제를 제공합니다.
 * @ingroup Editor
 */

#include "FContentBrowserWidget.h"
#include "FEditorWorkspace.h"
#include "FAssetImportController.h"
#include "FAssetEditorManager.h"
#include "Editor/FImGuiUIScale.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FAssetDragDrop.h"
#include "EditorStyle.h"
#include "Core/FilePath.h"
#include "Engine/FProject.h"
#include "Engine/Cascade/FParticleSystemAssetManager.h"
#include "Asset/UTexture2D.h"
#include "Asset/UMaterial.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Viewer/FAssetViewer.h"
#include "RenderCore/IRendererModule.h"
#include "Platform/Window.h"
#include "Platform/WindowResources.h"
#include "Core/Types.h"
#include <d3d11.h>
#include <shellapi.h>
#include <imgui_internal.h>
#include <algorithm>
#include <format>
#include <fstream>
#include <span>

using namespace Editor;

/**
 * @brief FContentBrowserWidget 구현에 한정되는 내부 형식·상수·도우미 함수입니다.
 */
namespace {

    /**
     * @brief 콘텐츠 종류별 리소스 아이콘, 대체 글리프와 표시 색상입니다.
     * @ingroup Editor
     */
    struct FContentIcon {

        /**
         * @brief 콘텐츠 종류를 식별하는 문자열입니다.
         */
        std::string_view mKind{};

        /**
         * @brief 아이콘의 마우스 오버 설명입니다.
         */
        const char* mTooltip{};

        /**
         * @brief 실행 파일에 포함된 아이콘 이미지 리소스 ID입니다.
         */
        WORD mResourceId{};

        /**
         * @brief 리소스 이미지가 없을 때 사용할 아이콘 글리프입니다.
         */
        Editor::EditorStyle::EIcon mFallback{};

        /**
         * @brief 아이콘에 곱할 표시 색상입니다.
         */
        ImU32 mTint{IM_COL32_WHITE};
    };

    /**
     * @brief 알 수 없는 파일 종류에 사용할 기본 아이콘입니다.
     */
    constexpr FContentIcon FileIcon{"File", "파일", MacawContentFileIconResource, Editor::EditorStyle::EIcon::Component};

    /**
     * @brief 콘텐츠 종류별 아이콘 메타데이터 목록입니다.
     */
    constexpr FContentIcon ContentIcons[]{
        FileIcon,
        {"Lua Script", "Lua 스크립트", MacawContentLuaIconResource, Editor::EditorStyle::EIcon::Component, IM_COL32(164, 197, 255, 255)},
        {"Shader Source", "셰이더 소스", MacawContentFileIconResource, Editor::EditorStyle::EIcon::Component, IM_COL32(120, 214, 236, 255)},
        {"Folder", "폴더", MacawContentFolderIconResource, Editor::EditorStyle::EIcon::Component, IM_COL32(255, 208, 111, 255)},
        {"Mesh", "메시", MacawContentMeshIconResource, Editor::EditorStyle::EIcon::Local},
        {"Skeletal Mesh", "스켈레탈 메시", MacawContentSkeletalMeshIconResource, Editor::EditorStyle::EIcon::Skeleton},
        {"Skeleton", "스켈레톤", MacawContentSkeletonIconResource, Editor::EditorStyle::EIcon::Skeleton},
        {"Material", "머티리얼", MacawContentMaterialIconResource, Editor::EditorStyle::EIcon::Grid},
        {"Texture", "텍스처", MacawContentTextureIconResource, Editor::EditorStyle::EIcon::Sprite},
        {"Sequence", "시퀀스", MacawContentSequenceIconResource, Editor::EditorStyle::EIcon::Sequence},
        {"Animation Sequence", "애니메이션 시퀀스", MacawContentAnimationSequenceIconResource, Editor::EditorStyle::EIcon::Sequence},
        {"Blend Space", "블렌드 스페이스", MacawContentAnimationSequenceIconResource, Editor::EditorStyle::EIcon::Sequence, IM_COL32(120, 214, 236, 255)},
        {"Anim Graph", "애니메이션 그래프", MacawContentAnimationSequenceIconResource, Editor::EditorStyle::EIcon::Sequence, IM_COL32(150, 165, 255, 255)},
        {"Particle System", "파티클 시스템", MacawContentFileIconResource, Editor::EditorStyle::EIcon::Sprite, IM_COL32(255, 190, 110, 255)},
        {"Scene", "씬", MacawContentSceneIconResource, Editor::EditorStyle::EIcon::World}};

    bool IsShaderSource(const std::filesystem::path& Path) {
        const auto Extension{Core::FilePath::ToLowerExtension(Path)};

        return Extension == L".hlsl" || Extension == L".hlsli";
    }

    const char* GetAssetKind(Asset::EAssetType Type) {
        switch (Type) {
        case Asset::EAssetType::Mesh:
            return "Mesh";
        case Asset::EAssetType::SkeletalMesh:
            return "Skeletal Mesh";
        case Asset::EAssetType::Skeleton:
            return "Skeleton";
        case Asset::EAssetType::Shader:
            return "Shader";
        case Asset::EAssetType::Material:
            return "Material";
        case Asset::EAssetType::PhysicalMaterial:
            return "Physical Material";
        case Asset::EAssetType::Texture:
            return "Texture";
        case Asset::EAssetType::Sequence:
            return "Sequence";
        case Asset::EAssetType::AnimSequence:
            return "Animation Sequence";
        case Asset::EAssetType::BlendSpace:
            return "Blend Space";
        case Asset::EAssetType::AnimGraph:
            return "Anim Graph";
        case Asset::EAssetType::ParticleSystem:
            return "Particle System";
        default:
            return "Scene";
        }
    }

    /**
     * @brief 콘텐츠 종류에 대응하는 아이콘 메타데이터를 조회합니다.
     * @param Kind 콘텐츠 종류 문자열입니다.
     * @return 정적 아이콘 항목의 참조이며 모르는 종류는 기본 파일 아이콘입니다.
     */
    const FContentIcon& GetContentIcon(std::string_view Kind) {
        for (const FContentIcon& Icon : ContentIcons) {
            if (Icon.mKind == Kind) {
                return Icon;
            }
        }

        return FileIcon;
    }

    /**
     * @brief 썸네일·리소스 아이콘·대체 글리프 중 사용 가능한 이미지를 표시합니다.
     * @param Size 출력 또는 표시 영역 크기(픽셀)입니다.
     * @param Thumbnail 캐시에서 제공한 썸네일 네이티브 텍스처 핸들입니다.
     * @param Resource 썸네일이 없을 때 사용할 GPU 아이콘 리소스입니다.
     * @param Icon 리소스가 없을 때 사용할 대체 아이콘 정보입니다.
     */
    void DrawContentImage(float Size, void* Thumbnail, ID3D11ShaderResourceView* Resource, const FContentIcon& Icon) {
        if (Thumbnail != nullptr) {
            ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Thumbnail))}, {Size, Size});

            return;
        }

        const float IconSize{Size * 0.8f};
        const float Inset{(Size - IconSize) * 0.5f};
        const ImVec2 Cursor{ImGui::GetCursorScreenPos()};
        const ImVec2 Origin{Cursor.x + Inset, Cursor.y + Inset};
        ImDrawList* DrawList{ImGui::GetWindowDrawList()};

        if (Resource != nullptr) {
            DrawList->AddImage(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Resource))}, Origin, {Origin.x + IconSize, Origin.y + IconSize}, {0.0f, 0.0f}, {1.0f, 1.0f}, Icon.mTint);
        } else if (Icon.mKind == "Folder") {
            DrawList->AddRectFilled({Origin.x, Origin.y + IconSize * 0.22f}, {Origin.x + IconSize * 0.45f, Origin.y + IconSize * 0.45f}, Icon.mTint, IconSize * 0.04f);
            DrawList->AddRectFilled({Origin.x, Origin.y + IconSize * 0.33f}, {Origin.x + IconSize, Origin.y + IconSize * 0.83f}, Icon.mTint, IconSize * 0.05f);
        } else {
            Editor::EditorStyle::DrawIcon(Icon.mFallback, Origin, IconSize, Icon.mTint);
        }

        ImGui::Dummy({Size, Size});
    }
}

FContentBrowserWidget::FContentBrowserWidget(FEditorContext* Context, Platform::FWindowsApplication* Platform, FEditorAssetCatalog& AssetCatalog, FAssetThumbnailCache& Thumbnails, FImGuiUIScale& UIScale, FEditorWorkspace& Workspace, FAssetImportController& Imports, FAssetEditorManager& AssetEditors)
	: FEditorContextWidget{Context, "ContentBrowser"},
	  mContext{Context},
	  mPlatform{Platform},
	  mAssetCatalog{&AssetCatalog},
	  mThumbnails{&Thumbnails},
	  mUIScale{UIScale},
	  mWorkspace{Workspace},
	  mImports{Imports},
	  mAssetEditors{AssetEditors} {

}

FContentBrowserWidget::~FContentBrowserWidget() = default;

void FContentBrowserWidget::Tick() {
    if (mRefreshContent) {
        RefreshContent();
    }
}

void FContentBrowserWidget::DrawDragPreview() {
    const ImGuiPayload* Payload{ImGui::GetDragDropPayload()};

    if (Payload == nullptr || Payload->SourceId != mAssetDragSourceId || Payload->IsDelivery()) {
        return;
    }

    const auto Path{Core::FilePath::FromUtf8(static_cast<const char*>(Payload->Data))};
    const auto Type{mAssetTypes.find(Path)};
    const bool HasThumbnail{Type != mAssetTypes.end() && Viewer::FAssetViewer::Supports(Type->second)};
    void* Thumbnail{HasThumbnail ? mThumbnails->GetThumbnail(Path, Type->second) : nullptr};
    const char* Kind{Type != mAssetTypes.end() ? GetAssetKind(Type->second) : Core::FilePath::ToLowerExtension(Path) == L".lua" ? "Lua Script" : "File"};
    const FContentIcon& Icon{GetContentIcon(Kind)};
    const auto CachedIcon{mContentIcons.find(Icon.mKind)};
    ID3D11ShaderResourceView* Resource{CachedIcon != mContentIcons.end() ? CachedIcon->second.Get() : nullptr};
    const Core::FString Name{Core::FilePath::ToUtf8(Path.stem())};

    if (ImGui::BeginTooltip()) {
        DrawContentImage(64.0f * mUIScale.GetScale(), Thumbnail, Resource, Icon);
        ImGui::TextUnformatted(Name.c_str());
        ImGui::EndTooltip();
    }
}

const std::filesystem::path& FContentBrowserWidget::GetDirectory() const {
    return mContentDirectory;
}

const std::filesystem::path& FContentBrowserWidget::GetSelectedPath() const {
    return mSelectedContentPath;
}

void FContentBrowserWidget::MarkRefresh() {
    mRefreshContent = true;
}

void FContentBrowserWidget::SetCatalogChanged(std::function<void()> Handler) {
    mCatalogChanged = std::move(Handler);
}

void FContentBrowserWidget::Reveal(const std::filesystem::path& Path) {
    mContentFilter.reset();
    mContentSearch.fill('\0');
    NavigateContent(Path.parent_path());
    mSelectedContentPath = Path;
    mWorkspace.FocusContentBrowser();
}

void FContentBrowserWidget::OnImported(const std::filesystem::path& Path, Uint32 Count, const Core::FString& Error) {
    mContentError = Error;
    if (Count != 0) {
        mContentFilter.reset();
        mContentSearch.fill('\0');
        MarkRefresh();
        RefreshContent();
        NavigateContent(Count == 1 ? Path.parent_path() : Path);
        mSelectedContentPath = Path;
        mContentError = Error;
    }
}

void FContentBrowserWidget::DrawContent(float) {
    DrawContent();
}

void FContentBrowserWidget::LoadContentIcons(RenderCore::IRendererModule* Renderer) {
    auto* Device{static_cast<ID3D11Device*>(Renderer->GetNativeDevice())};

    if (Device == nullptr) {
        return;
    }

    const HMODULE Module{GetModuleHandleW(nullptr)};
    const std::filesystem::path ImageName{L"ContentIcon.png"};

    for (const FContentIcon& Icon : ContentIcons) {
        const HRSRC Resource{FindResourceW(Module, MAKEINTRESOURCEW(Icon.mResourceId), RT_RCDATA)};
        const HGLOBAL Data{Resource != nullptr ? LoadResource(Module, Resource) : nullptr};
        const auto* Bytes{Data != nullptr ? static_cast<const Uint8*>(LockResource(Data)) : nullptr};

        if (Bytes == nullptr) {
            continue;
        }

        const std::span<const Uint8> Image{Bytes, SizeofResource(Module, Resource)};
        Asset::UTexture2D Texture{};

        if (Texture.LoadFromMemory(Image, ImageName, false) && Texture.CreateResources(Device)) {
            mContentIcons.try_emplace(Icon.mKind, Texture.GetShaderResourceView());
        }
    }
}

void FContentBrowserWidget::DrawContent() {
    const ImVec2 Minimum{ImGui::GetWindowPos()};
    const ImVec2 Size{ImGui::GetWindowSize()};

    mImports.RegisterContentDropTarget(mContentDirectory, {Minimum.x, Minimum.y, Minimum.x + Size.x, Minimum.y + Size.y});
    ImGui::TextUnformatted(mContext->GetProject()->GetName().c_str());
    ImGui::SameLine();

    if (EditorStyle::SmallButton("Refresh")) {
        mRefreshContent = true;
    }

    ImGui::Separator();
    DrawContentBreadcrumbs();
    ImGui::SetNextItemWidth(120.0f);

    const char* FilterName{!mContentFilter.has_value() ? "All Assets" : *mContentFilter == Asset::EAssetType::Mesh         ? "Meshes" : *mContentFilter == Asset::EAssetType::SkeletalMesh ? "Skeletal Meshes" : *mContentFilter == Asset::EAssetType::Skeleton ? "Skeletons" : *mContentFilter == Asset::EAssetType::Shader ? "Shaders" : *mContentFilter == Asset::EAssetType::Material ? "Materials" : *mContentFilter == Asset::EAssetType::PhysicalMaterial ? "Physical Materials" : *mContentFilter == Asset::EAssetType::Texture ? "Textures" : *mContentFilter == Asset::EAssetType::Sequence ? "Sequences" : *mContentFilter == Asset::EAssetType::AnimSequence ? "Animation Sequences" : *mContentFilter == Asset::EAssetType::BlendSpace ? "Blend Spaces" : *mContentFilter == Asset::EAssetType::AnimGraph ? "Anim Graphs" : *mContentFilter == Asset::EAssetType::ParticleSystem ? "Particle Systems" : "Scenes"};

    if (ImGui::BeginCombo("##AssetType", FilterName)) {
        if (ImGui::Selectable("All Assets", !mContentFilter.has_value())) {
            mContentFilter.reset();
        }

        for (Asset::EAssetType Type : {Asset::EAssetType::Mesh, Asset::EAssetType::SkeletalMesh, Asset::EAssetType::Skeleton, Asset::EAssetType::Material, Asset::EAssetType::PhysicalMaterial, Asset::EAssetType::Shader, Asset::EAssetType::Texture, Asset::EAssetType::Scene, Asset::EAssetType::Sequence, Asset::EAssetType::AnimSequence, Asset::EAssetType::BlendSpace, Asset::EAssetType::AnimGraph, Asset::EAssetType::ParticleSystem}) {
            const char* Name{Type == Asset::EAssetType::Mesh ? "Meshes" : Type == Asset::EAssetType::SkeletalMesh ? "Skeletal Meshes" : Type == Asset::EAssetType::Skeleton ? "Skeletons" : Type == Asset::EAssetType::Shader ? "Shaders" : Type == Asset::EAssetType::Material ? "Materials" : Type == Asset::EAssetType::PhysicalMaterial ? "Physical Materials" : Type == Asset::EAssetType::Texture ? "Textures" : Type == Asset::EAssetType::Sequence ? "Sequences" : Type == Asset::EAssetType::AnimSequence ? "Animation Sequences" : Type == Asset::EAssetType::BlendSpace ? "Blend Spaces" : Type == Asset::EAssetType::AnimGraph ? "Anim Graphs" : Type == Asset::EAssetType::ParticleSystem ? "Particle Systems" : "Scenes"};

            if (ImGui::Selectable(Name, mContentFilter == Type)) {
                mContentFilter = Type;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x));
    ImGui::InputTextWithHint("##ContentSearch", "Search assets...", mContentSearch.data(), mContentSearch.size());

    if (!mContentError.empty()) {
        ImGui::TextWrapped("%s", mContentError.c_str());
    }

    // 테이블 행의 위아래 셀 여백만큼 줄여 창 스크롤바가 생기지 않게 합니다.
    const float Height{std::max(64.0f, ImGui::GetContentRegionAvail().y - ImGui::GetStyle().CellPadding.y * 2.0f)};

    if (ImGui::BeginTable("Content Browser", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 190.0f);
        ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::BeginChild("Content Folders", ImVec2{0.0f, Height});
        DrawContentTree({});

        mRevealContentDirectory = false;

        if (ImGui::BeginPopupContextWindow("ContentFolderMenu")) {
            DrawContentContextMenu();
            ImGui::EndPopup();
        }

        ImGui::EndChild();
        ImGui::TableSetColumnIndex(1);
        ImGui::BeginChild("Content Files", ImVec2{0.0f, Height});
        DrawContentEntries();

        if (ImGui::IsWindowFocused() && !ImGui::GetIO().WantTextInput && !mSelectedContentPath.empty() && mContext->CanEditWorld()) {
            if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
                BeginContentAction(EContentAction::Rename);
            } else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
                BeginContentAction(EContentAction::Delete);
            }
        }

        if (ImGui::BeginPopupContextWindow("ContentFileMenu")) {
            DrawContentContextMenu();
            ImGui::EndPopup();
        }

        ImGui::EndChild();
        ImGui::EndTable();
    }

    DrawContentActionPopup();
}

std::filesystem::path FContentBrowserWidget::GetContentActionPath(const std::filesystem::path& Path) const {
    return mContext->GetProject()->GetAssets()->GetContentRoot() / Path;
}

void FContentBrowserWidget::BeginContentAction(EContentAction Action) {
    mContentAction = Action;
    mContentActionTarget = mSelectedContentPath;
    mContentActionError.clear();
    mContentActionReferences.clear();
    mOpenContentAction = true;

    const std::filesystem::path Absolute{GetContentActionPath(mContentActionTarget)};
    std::error_code Code{};
    const bool Directory{std::filesystem::is_directory(Absolute, Code)};
    const Core::FString Name{Core::FilePath::ToUtf8(Directory ? mContentActionTarget.filename() : mContentActionTarget.stem())};

    mContentActionName.fill('\0');
    std::copy_n(Name.begin(), std::min(Name.size(), mContentActionName.size() - 1), mContentActionName.begin());

    // 에셋은 다른 에셋에 상대 경로 문자열로 저장됩니다.
    const Core::FString Needle{'"' + Core::FilePath::ToGenericUtf8(mContentActionTarget) + (Directory ? '/' : '"')};

    const std::filesystem::path Root{mContext->GetProject()->GetAssets()->GetContentRoot()};
    std::filesystem::recursive_directory_iterator Iterator{Root, std::filesystem::directory_options::skip_permission_denied, Code};
    const std::filesystem::recursive_directory_iterator End{};

    for (; !Code && Iterator != End; Iterator.increment(Code)) {
        const std::filesystem::path Relative{Iterator->path().lexically_relative(Root)};
        const auto Inside{Relative.lexically_relative(mContentActionTarget)};

        if (!Iterator->is_regular_file(Code) || Core::FilePath::ToLowerExtension(Relative) != L".masset" || Relative == mContentActionTarget || (!Inside.empty() && *Inside.begin() != L"..")) {
            continue;
        }

        std::ifstream Input{Iterator->path(), std::ios::binary};
        const std::string Text{std::istreambuf_iterator<char>{Input}, std::istreambuf_iterator<char>{}};

        if (Text.find(Needle) != std::string::npos) {
            mContentActionReferences.push_back(Relative);
        }
    }
}

Core::FString FContentBrowserWidget::FindContentActionBlocker(const std::filesystem::path& Absolute) const {
    Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    Core::FString Error{};
    std::error_code Code{};
    const auto Contains{[&](const std::filesystem::path& Open) {
        if (Open.empty()) {
            return false;
        }

        const std::filesystem::path Resolved{Assets->ResolvePath(Open, Error)};
        const auto Relative{std::filesystem::weakly_canonical(Resolved, Code).lexically_relative(std::filesystem::weakly_canonical(Absolute, Code))};

        return !Resolved.empty() && !Relative.empty() && *Relative.begin() != L"..";
    }};

    if (!mContext->CanEditWorld()) {
        return "PIE 중에는 바꿀 수 없습니다.";
    }

    if (Contains(mContext->GetScenePath())) {
        return "지금 열려 있는 씬입니다. 다른 씬을 연 뒤에 다시 시도하세요.";
    }

    if (mAssetEditors.HasOpenMaterial(Contains)) {
        return "편집 중인 머티리얼입니다. 머티리얼 편집기를 닫은 뒤에 다시 시도하세요.";
    }

    if (mAssetEditors.HasUnsavedSource(Contains)) {
        return "편집기에서 소스 파일의 변경 사항을 저장하거나 취소한 뒤 다시 시도하세요.";
    }

    return {};
}

void FContentBrowserWidget::DrawContentActionPopup() {
    constexpr const char* RenameName{"Rename Asset"};
    constexpr const char* DeleteName{"Delete Asset"};
    const char* PopupName{mContentAction == EContentAction::Rename ? RenameName : DeleteName};

    if (mOpenContentAction) {
        ImGui::OpenPopup(PopupName);

        mOpenContentAction = false;
    }

    if (!ImGui::BeginPopupModal(PopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    const std::filesystem::path Absolute{GetContentActionPath(mContentActionTarget)};
    std::error_code Code{};
    const bool Directory{std::filesystem::is_directory(Absolute, Code)};
    const Core::FString Display{Core::FilePath::ToGenericUtf8(mContentActionTarget)};
    bool Confirmed{};

    if (mContentAction == EContentAction::Rename) {
        ImGui::Text("%s의 새 이름", Display.c_str());

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        Confirmed = ImGui::InputText("##ContentRename", mContentActionName.data(), mContentActionName.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    } else {
        ImGui::Text("%s%s을(를) 휴지통으로 옮길까요?", Display.c_str(), Directory ? " 폴더와 그 안의 모든 파일" : "");
    }

    if (!mContentActionReferences.empty()) {
        ImGui::Spacing();
        ImGui::TextColored({1.0f, 0.75f, 0.3f, 1.0f}, "다음 에셋이 이 경로를 참조합니다. 참조가 끊어집니다.");

        for (Index_t Index{}; Index < std::min<std::size_t>(mContentActionReferences.size(), 6); ++Index) {
            ImGui::BulletText("%s", Core::FilePath::ToGenericUtf8(mContentActionReferences[Index]).c_str());
        }

        if (mContentActionReferences.size() > 6) {
            ImGui::TextDisabled("외 %zu개", mContentActionReferences.size() - 6);
        }
    }

    if (!mContentActionError.empty()) {
        ImGui::TextColored({1.0f, 0.45f, 0.4f, 1.0f}, "%s", mContentActionError.c_str());
    }

    ImGui::Spacing();

    Confirmed |= EditorStyle::Button(mContentAction == EContentAction::Rename ? "Rename" : "Delete");

    ImGui::SameLine();

    if (EditorStyle::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
    }

    if (Confirmed && (mContentAction == EContentAction::Rename ? RenameContent() : DeleteContent())) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

bool FContentBrowserWidget::RenameContent() {
    const std::filesystem::path Absolute{GetContentActionPath(mContentActionTarget)};
    const Core::FString NewName{mContentActionName.data()};
    std::error_code Code{};
    const bool Directory{std::filesystem::is_directory(Absolute, Code)};

    if (mContentActionError = FindContentActionBlocker(Absolute); !mContentActionError.empty()) {
        return false;
    }

    if (NewName.empty() || NewName.find_first_of("\\/:*?\"<>|") != Core::FString::npos || NewName.back() == '.' || NewName.back() == ' ') {
        mContentActionError = "이름이 비어 있거나 쓸 수 없는 문자가 들어 있습니다.";

        return false;
    }

    const std::filesystem::path NewFile{Directory ? std::filesystem::path{Core::FilePath::FromUtf8(NewName.c_str())} : std::filesystem::path{Core::FilePath::FromUtf8(NewName.c_str())} += mContentActionTarget.extension()};
    const std::filesystem::path Destination{Absolute.parent_path() / NewFile};
    if (Destination == Absolute) {
        return true;
    }

    // 대소문자만 바꾸는 경우는 같은 파일로 보이므로 존재 검사를 건너뜁니다.
    if (std::filesystem::exists(Destination, Code) && !std::filesystem::equivalent(Destination, Absolute, Code)) {
        mContentActionError = "같은 이름의 항목이 이미 있습니다.";

        return false;
    }

    std::filesystem::rename(Absolute, Destination, Code);

    if (Code) {
        mContentActionError = "이름을 바꾸지 못했습니다: " + Code.message();

        return false;
    }

    // 옛 경로의 캐시 객체는 바뀌지 않으므로 은퇴시켜, 나중에 같은 경로를 다시 쓸 때 새로 로드하게 합니다.
    mContext->GetProject()->GetParticleSystems()->RetireAssets(Absolute);
    mContext->GetProject()->GetAssets()->RetireAssets(Absolute);

    mAssetEditors.CloseRetired(Absolute);
    mSelectedContentPath = mContentActionTarget.parent_path() / NewFile;
    mRefreshContent = true;

    if (mContentDirectory == mContentActionTarget) {
        mContentDirectory = mSelectedContentPath;
    }

    mContext->AddLog("Renamed " + Core::FilePath::ToGenericUtf8(mContentActionTarget) + " to " + Core::FilePath::ToUtf8(NewFile));

    return true;
}

bool FContentBrowserWidget::DeleteContent() {
    const std::filesystem::path Absolute{GetContentActionPath(mContentActionTarget)};

    if (mContentActionError = FindContentActionBlocker(Absolute); !mContentActionError.empty()) {
        return false;
    }

    // 실수로 지워도 되돌릴 수 있도록 휴지통으로 옮깁니다. 경로 목록은 두 개의 NUL로 끝나야 합니다.
    std::wstring From{Absolute.native()};
    From.push_back(L'\0');

    SHFILEOPSTRUCTW Operation{};
    Operation.hwnd = mPlatform->GetWindowHandle();
    Operation.wFunc = FO_DELETE;
    Operation.pFrom = From.c_str();
    Operation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;

    if (SHFileOperationW(&Operation) != 0 || Operation.fAnyOperationsAborted) {
        mContentActionError = "휴지통으로 옮기지 못했습니다. 다른 프로그램이 파일을 쓰고 있는지 확인하세요.";

        return false;
    }

    mSelectedContentPath.clear();

    // 지운 에셋을 참조하던 객체는 살려 두고, 같은 경로에 다시 임포트하면 새 객체를 로드하게 합니다.
    mContext->GetProject()->GetParticleSystems()->RetireAssets(Absolute);
    mContext->GetProject()->GetAssets()->RetireAssets(Absolute);

    mAssetEditors.CloseRetired(Absolute);
    mRefreshContent = true;

    mContext->AddLog("Moved " + Core::FilePath::ToGenericUtf8(mContentActionTarget) + " to the Recycle Bin");

    return true;
}

void FContentBrowserWidget::DrawContentContextMenu() {
    const bool CanEdit{mContext->CanEditWorld()};

    if (ImGui::MenuItem("New Folder", nullptr, false, CanEdit)) {
        CreateContentFolder();
    }

    if (ImGui::MenuItem("New Material", nullptr, false, CanEdit)) {
        CreateMaterial();
    }

    if (ImGui::MenuItem("New Physical Material", nullptr, false, CanEdit)) {
        CreatePhysicalMaterial();
    }

    if (ImGui::MenuItem("New Blend Space 1D", nullptr, false, CanEdit)) {
        CreateBlendSpace();
    }

    if (ImGui::MenuItem("New Anim Graph", nullptr, false, CanEdit)) {
        CreateAnimGraph();
    }

    if (ImGui::MenuItem("New Particle System", nullptr, false, CanEdit)) {
        CreateParticleSystem();
    }

    if (ImGui::MenuItem("New Lua Script", nullptr, false, CanEdit)) {
        CreateLuaScript();
    }

    if (!mSelectedContentPath.empty()) {
        ImGui::Separator();

        if (ImGui::MenuItem("Rename...", "F2", false, CanEdit)) {
            BeginContentAction(EContentAction::Rename);
        }

        if (ImGui::MenuItem("Delete", "Del", false, CanEdit)) {
            BeginContentAction(EContentAction::Delete);
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Import...", nullptr, false, CanEdit)) {
        mImports.ImportAsset();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show in Explorer")) {
        ShowContentInExplorer();
    }

    if (ImGui::MenuItem("Refresh")) {
        mRefreshContent = true;
    }
}

void FContentBrowserWidget::CreateContentFolder() {
    const std::filesystem::path Root{mContext->GetProject()->GetAssets()->GetContentRoot()};
    std::filesystem::path Path{mContentDirectory / "NewFolder"};
    std::error_code Code{};

    for (Index_t Index{1}; std::filesystem::exists(Root / Path, Code) && !Code; ++Index) {
        Path = mContentDirectory / ("NewFolder_" + std::to_string(Index));
    }

    if (Code || !std::filesystem::create_directory(Root / Path, Code)) {
        mContext->AddLog("Cannot create the folder " + Core::FilePath::ToGenericUtf8(Path));

        return;
    }

    mRefreshContent = true;
    mSelectedContentPath = Path;
}

void FContentBrowserWidget::ShowContentInExplorer() const {
    std::filesystem::path Target{mContext->GetProject()->GetAssets()->GetContentRoot() / (!mSelectedContentPath.empty() ? mSelectedContentPath : mContentDirectory)};
    std::error_code Code{};

    Target = std::filesystem::weakly_canonical(Target, Code).make_preferred();

    // 파일은 상위 폴더를 열고 그 파일을 선택하며, 폴더는 그 폴더를 엽니다.
    const std::wstring Arguments{std::filesystem::is_directory(Target, Code) ? L"\"" + Target.native() + L"\"" : L"/select,\"" + Target.native() + L"\""};

    ShellExecuteW(nullptr, L"open", L"explorer.exe", Arguments.c_str(), nullptr, SW_SHOWNORMAL);
}

void FContentBrowserWidget::RefreshContent() {
    mAssetPaths.clear();
    mAssetTypes.clear();
    mContentFolders.clear();
    mContentFolders[{}] = {};
    mContentError.clear();

    if (mAssetCatalog == nullptr) {
        return;
    }

    mAssetCatalog->Refresh(mContentError);

    for (const auto& Directory : mAssetCatalog->GetDirectories()) {
        mContentFolders[Directory.parent_path()].push_back(Directory);
        mContentFolders.try_emplace(Directory);
    }

    for (const auto& Entry : mAssetCatalog->GetEntries()) {
        mAssetPaths.push_back(Entry.GetPath());

        if (Entry.GetType().has_value()) {
            mAssetTypes.emplace(Entry.GetPath(), *Entry.GetType());
        }
    }

    for (auto& Entry : mContentFolders) {
        std::sort(Entry.second.begin(), Entry.second.end());
        Entry.second.erase(std::unique(Entry.second.begin(), Entry.second.end()), Entry.second.end());
    }

    while (!mContentDirectory.empty() && !mContentFolders.contains(mContentDirectory)) {
        mContentDirectory = mContentDirectory.parent_path();
    }

    mThumbnails->Invalidate();
    if (mCatalogChanged) {
        mCatalogChanged();
    }

    mRefreshContent = false;
    mRevealContentDirectory = true;
}

void FContentBrowserWidget::NavigateContent(const std::filesystem::path& Directory) {
    if (mContentFolders.contains(Directory)) {
        mContentDirectory = Directory;
        mSelectedContentPath.clear();
        mRevealContentDirectory = true;
    }
}

void FContentBrowserWidget::DrawContentTree(const std::filesystem::path& Directory) {
    const auto Entry{mContentFolders.find(Directory)};

    if (Entry == mContentFolders.end()) {
        return;
    }

    const Core::FString Label{Directory.empty() ? "Content" : Core::FilePath::ToUtf8(Directory.filename())};
    const Core::FString Id{Core::FilePath::ToUtf8(Directory)};
    ImGuiTreeNodeFlags Flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth};

    if (Directory == mContentDirectory) {
        Flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (Entry->second.empty()) {
        Flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (Directory.empty()) {
        Flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    const auto Relative{mContentDirectory.lexically_relative(Directory)};

    if (mRevealContentDirectory && (Directory.empty() || Directory == mContentDirectory || (!Relative.empty() && *Relative.begin() != L".."))) {
        ImGui::SetNextItemOpen(true);
    }

    ImGui::PushID(Id.c_str());

    const bool Open{ImGui::TreeNodeEx(Label.c_str(), Flags)};
    const ImVec2 Minimum{ImGui::GetItemRectMin()};
    const ImVec2 Maximum{ImGui::GetItemRectMax()};

    mImports.RegisterContentDropTarget(Directory, {Minimum.x, Minimum.y, Maximum.x, Maximum.y});

    if (ImGui::IsItemClicked()) {
        NavigateContent(Directory);
    }

    if (Open) {
        for (const auto& Child : Entry->second) {
            DrawContentTree(Child);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FContentBrowserWidget::DrawContentBreadcrumbs() {
    ImGui::PushID("Content Path");

    ImGui::BeginDisabled(mContentDirectory.empty());

    if (EditorStyle::SmallButton("Up")) {
        NavigateContent(mContentDirectory.parent_path());
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if (EditorStyle::SmallButton("Content")) {
        NavigateContent({});
    }

    const std::filesystem::path Current{mContentDirectory};
    std::filesystem::path Path{};

    for (const auto& Part : Current) {
        Path /= Part;

        const Core::FString Label{Core::FilePath::ToUtf8(Part)};
        const float Right{ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x};

        if (Right - ImGui::GetItemRectMax().x > ImGui::CalcTextSize(Label.c_str()).x + 48.0f) {
            ImGui::SameLine();
        }

        ImGui::TextUnformatted("/");
        ImGui::SameLine();

        const Core::FString Id{Core::FilePath::ToUtf8(Path)};

        ImGui::PushID(Id.c_str());

        if (EditorStyle::SmallButton(Label.c_str())) {
            NavigateContent(Path);
        }

        ImGui::PopID();
    }

    ImGui::PopID();
}

void FContentBrowserWidget::DrawContentEntries() {
    const ImGuiTextFilter Search{mContentSearch.data()};
    const bool Recursive{mContentFilter.has_value() || mContentSearch[0] != '\0'};
    Core::TArray<std::filesystem::path> Folders{};
    Core::TArray<std::filesystem::path> Assets{};
    const auto Folder{mContentFolders.find(mContentDirectory)};

    if (!Recursive && Folder != mContentFolders.end()) {
        Folders = Folder->second;
    }

    for (const auto& Path : mAssetPaths) {
        const auto Type{mAssetTypes.find(Path)};
        const auto Relative{Path.lexically_relative(mContentDirectory)};
        const bool InFolder{Recursive ? mContentDirectory.empty() || (!Relative.empty() && *Relative.begin() != L"..") : Path.parent_path() == mContentDirectory};

        const bool MatchesFilter{!mContentFilter.has_value() || (Type != mAssetTypes.end() ? Type->second == *mContentFilter : *mContentFilter == Asset::EAssetType::Shader && IsShaderSource(Path))};

        if (!InFolder || !MatchesFilter || !Search.PassFilter(Core::FilePath::ToGenericUtf8(Path).c_str())) {
            continue;
        }

        Assets.push_back(Path);
    }

    const int Count{static_cast<int>(Folders.size() + Assets.size())};

    if (Count == 0) {
        ImGui::TextDisabled("No assets match this folder or search.");

        return;
    }

    const float Scale{mUIScale.GetScale()};
    const float Padding{4.0f * Scale};
    const bool Compact{ImGui::GetContentRegionAvail().y < 164.0f * Scale};
    const float TileWidth{(Compact ? 180.0f : 120.0f) * Scale};
    const float ImageSize{(Compact ? 48.0f : 96.0f) * Scale};
    const float ImageOffset{(Compact ? 4.0f : 10.0f) * Scale};
    const float TextX{Compact ? ImageOffset + ImageSize + 8.0f * Scale : Padding};
    const float TypeY{Compact ? Padding : ImageSize + Padding * 2.0f};
    const float NameY{TypeY + ImGui::GetTextLineHeightWithSpacing()};
    const float NameHeight{ImGui::GetTextLineHeight() * 2.0f + ImGui::GetStyle().ItemSpacing.y};
    const float TileHeight{std::max(Padding + ImageSize, NameY + NameHeight) + Padding * 2.0f};
    const int Columns{std::clamp(static_cast<int>(ImGui::GetContentRegionAvail().x / (TileWidth + 8.0f * Scale)), 1, 12)};
    bool Navigate{};
    std::filesystem::path Requested{};

    if (ImGui::BeginTable("ContentGrid", Columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX)) {
        for (Index_t Column{}; Column < Columns; ++Column) {
            ImGui::TableSetupColumn("##AssetColumn", ImGuiTableColumnFlags_WidthFixed, TileWidth);
        }

        ImGuiListClipper Clipper{};
        Clipper.Begin((Count + Columns - 1) / Columns, TileHeight);

        while (Clipper.Step()) {
            for (Index_t Row{static_cast<Index_t>(Clipper.DisplayStart)}; Row < Clipper.DisplayEnd; ++Row) {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, TileHeight);

                for (Index_t Column{}; Column < Columns; ++Column) {
                    const Index_t Index{static_cast<Index_t>(Row * Columns + Column)};

                    if (Index >= Count) {
                        break;
                    }

                    ImGui::TableSetColumnIndex(static_cast<int>(Column));

                    const bool Directory{Index < Folders.size()};
                    const auto& Path{Directory ? Folders[Index] : Assets[Index - Folders.size()]};
                    const auto Type{mAssetTypes.find(Path)};
                    const bool LuaScript{!Directory && Type == mAssetTypes.end() && Core::FilePath::ToLowerExtension(Path) == L".lua"};
                    const bool ShaderSource{!Directory && Type == mAssetTypes.end() && IsShaderSource(Path)};
                    const bool HasThumbnail{!Directory && Type != mAssetTypes.end() && Viewer::FAssetViewer::Supports(Type->second)};
                    void* Thumbnail{HasThumbnail ? mThumbnails->GetThumbnail(Path, Type->second) : nullptr};
                    const char* Kind{Directory ? "Folder" : Type != mAssetTypes.end() ? GetAssetKind(Type->second) : LuaScript ? "Lua Script" : ShaderSource ? "Shader Source" : "File"};
                    const Core::FString Actual{Core::FilePath::ToGenericUtf8(Path)};
                    const Core::FString Name{Core::FilePath::ToUtf8(Directory || ShaderSource ? Path.filename() : Path.stem())};
                    const FContentIcon& Icon{GetContentIcon(Kind)};
                    const auto CachedIcon{mContentIcons.find(Icon.mKind)};
                    ID3D11ShaderResourceView* Resource{CachedIcon != mContentIcons.end() ? CachedIcon->second.Get() : nullptr};

                    ImGui::PushID(Actual.c_str());

                    const ImVec2 Origin{ImGui::GetCursorScreenPos()};

                    if (EditorStyle::Selectable("##AssetTile", mSelectedContentPath == Path, ImGuiSelectableFlags_AllowDoubleClick, {TileWidth - Padding, TileHeight - Padding * 2.0f})) {
                        mSelectedContentPath = Path;

                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                            if (Directory) {
                                Requested = Path;
                                Navigate = true;
                            } else if (LuaScript) {
                                mAssetEditors.OpenLuaScript(Path);
                            } else if (ShaderSource) {
                                mAssetEditors.OpenShaderSource(Path);
                            } else if (Type != mAssetTypes.end() && mContext->CanEditWorld()) {
                                mAssetEditors.OpenContentAsset(Path, Type->second);
                            }
                        }
                    }

                    // 우클릭 메뉴가 이 항목을 대상으로 하도록 먼저 선택합니다.
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                        mSelectedContentPath = Path;
                    }

                    const bool Hovered{ImGui::IsItemHovered()};

                    if (Directory) {
                        const ImVec2 Minimum{ImGui::GetItemRectMin()};
                        const ImVec2 Maximum{ImGui::GetItemRectMax()};

                        mImports.RegisterContentDropTarget(Path, {Minimum.x, Minimum.y, Maximum.x, Maximum.y});
                    }

                    // 애니메이션 시퀀스는 썸네일이 없지만 Details의 Animation 필드에 놓을 수 있도록 끌기를 허용합니다.
                    const bool AnimSequence{!Directory && Type != mAssetTypes.end() && Type->second == Asset::EAssetType::AnimSequence};
                    const bool BlendSpace{!Directory && Type != mAssetTypes.end() && Type->second == Asset::EAssetType::BlendSpace};
                    const bool AnimGraph{!Directory && Type != mAssetTypes.end() && Type->second == Asset::EAssetType::AnimGraph};

                    const bool Shader{!Directory && Type != mAssetTypes.end() && Type->second == Asset::EAssetType::Shader};

                    if ((LuaScript || HasThumbnail || AnimSequence || BlendSpace || AnimGraph || Shader) && mContext->CanEditWorld() && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
                        const char* Payload{LuaScript ? Editor::EditorAssetDragDrop::LuaScriptPayload : Shader ? Editor::EditorAssetDragDrop::ShaderPayload : AnimSequence ? Editor::EditorAssetDragDrop::AnimSequencePayload : BlendSpace ? Editor::EditorAssetDragDrop::BlendSpacePayload : AnimGraph ? Editor::EditorAssetDragDrop::AnimGraphPayload : Type->second == Asset::EAssetType::Mesh || Type->second == Asset::EAssetType::SkeletalMesh ? Editor::EditorAssetDragDrop::MeshPayload : Type->second == Asset::EAssetType::Skeleton ? Editor::EditorAssetDragDrop::SkeletonPayload : Type->second == Asset::EAssetType::Material ? Editor::EditorAssetDragDrop::MaterialPayload : Type->second == Asset::EAssetType::PhysicalMaterial ? Editor::EditorAssetDragDrop::PhysicalMaterialPayload : Editor::EditorAssetDragDrop::TexturePayload};

                        ImGui::SetDragDropPayload(Payload, Actual.c_str(), Actual.size() + 1);
                        mAssetDragSourceId = ImGui::GetDragDropPayload()->SourceId;
                        ImGui::EndDragDropSource();
                    }

                    if (Hovered) {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(Core::FilePath::ToGenericUtf8(Path).c_str());
                        ImGui::TextDisabled("%s", Icon.mTooltip);
                        ImGui::EndTooltip();
                    }

                    const ImVec2 Next{ImGui::GetCursorScreenPos()};

                    ImGui::SetCursorScreenPos({Origin.x + ImageOffset, Origin.y + Padding});
                    DrawContentImage(ImageSize, Thumbnail, Resource, Icon);
                    ImGui::SetCursorScreenPos({Origin.x + TextX, Origin.y + TypeY});
                    ImGui::TextDisabled("%s", Kind);
                    ImGui::SetCursorScreenPos({Origin.x + TextX, Origin.y + NameY});
                    ImGui::PushClipRect({Origin.x + TextX, Origin.y + NameY}, {Origin.x + TileWidth - Padding, Origin.y + NameY + NameHeight}, true);
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + TileWidth - TextX - 10.0f * Scale);
                    ImGui::TextWrapped("%s", Name.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::PopClipRect();
                    ImGui::SetCursorScreenPos(Next);
                    ImGui::Dummy({0.0f, 0.0f});
                    ImGui::PopID();
                }
            }
        }

        ImGui::EndTable();
    }

    if (Navigate) {
        NavigateContent(Requested);
    }
}

void FContentBrowserWidget::CreateMaterial() {
    mAssetEditors.ConfirmClose([this, Directory = mContentDirectory]() {
        CreateMaterialIn(Directory);
    });
}

void FContentBrowserWidget::CreateMaterialIn(const std::filesystem::path& Directory) {
    Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    std::filesystem::path Path{Directory / "Material.masset"};
    std::error_code Code{};

    for (Index_t Index{1}; std::filesystem::exists(Assets->GetContentRoot() / Path, Code) && !Code; ++Index) {
        Path = Directory / ("Material_" + std::to_string(Index) + ".masset");
    }

    auto Material{CoreUObject::NewObject<Asset::UMaterial>()};
    Material->SetName(Core::FilePath::ToUtf8(Path.stem()));

    if (Code || !Assets->SaveMaterial(Path, *Material, mContentError)) {
        mContext->AddLog(mContentError.empty() ? "Cannot create the material" : mContentError);

        return;
    }

    mRefreshContent = true;
    mSelectedContentPath = Path;

    mAssetEditors.OpenContentAsset(Path, Asset::EAssetType::Material);
}

void FContentBrowserWidget::CreatePhysicalMaterial() {
    Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    const auto Path{Assets->MakeUniqueAssetPath(mContentDirectory, "PhysicalMaterial", mContentError)};
    const PhysicsRuntime::UPhysicalMaterial Material{};

    if (Path.empty() || !Assets->SavePhysicalMaterial(Path, Material, mContentError)) {
        mContext->AddLog(mContentError);

        return;
    }

    mRefreshContent = true;
    mSelectedContentPath = Core::FilePath::FromUtf8(Assets->GetRelativePath(Path, mContentError));
    mAssetEditors.OpenContentAsset(mSelectedContentPath, Asset::EAssetType::PhysicalMaterial);
}

void FContentBrowserWidget::CreateAnimGraph() {
    Core::FString Error{};
    std::filesystem::path Path{};

    if (mContext->GetProject()->GetAssets()->CreateAnimGraph(mContentDirectory, "AnimGraph", Path, Error) == nullptr) {
        mContext->AddLog(Error.empty() ? "Cannot create the anim graph" : Error);

        return;
    }

    mRefreshContent = true;
    mSelectedContentPath = Path;

    mAssetEditors.OpenContentAsset(Path, Asset::EAssetType::AnimGraph);
}

void FContentBrowserWidget::CreateBlendSpace() {
    Core::FString Error{};
    std::filesystem::path Path{};

    if (mContext->GetProject()->GetAssets()->CreateBlendSpace1D(mContentDirectory, "BlendSpace", Path, Error) == nullptr) {
        mContext->AddLog(Error.empty() ? "Cannot create the blend space" : Error);

        return;
    }

    mRefreshContent = true;
    mSelectedContentPath = Path;

    mAssetEditors.OpenContentAsset(Path, Asset::EAssetType::BlendSpace);
}

void FContentBrowserWidget::CreateParticleSystem() {
    Core::FString Error{};
    std::filesystem::path Path{};

    if (mContext->GetProject()->GetParticleSystems()->CreateParticleSystem(mContentDirectory, "ParticleSystem", Path, Error) == nullptr) {
        mContext->AddLog(Error.empty() ? "Cannot create the particle system" : Error);

        return;
    }

    mRefreshContent = true;
    mSelectedContentPath = Path;
}

void FContentBrowserWidget::CreateLuaScript() {
    auto Template{GetContentActionPath(L"Scripts/template.lua")};
    std::error_code Code{};

    if (!std::filesystem::is_regular_file(Template, Code)) {
        Core::TStaticArray<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};

        if (Length > 0 && Length < Executable.size()) {
            Template = std::filesystem::path{Executable.data()}.parent_path() / L"Templates/Project/Content/Scripts/template.lua";
        }
    }

    auto Path{mContentDirectory / L"NewScript.lua"};

    for (Index_t Index{1}; !std::filesystem::copy_file(Template, GetContentActionPath(Path), std::filesystem::copy_options::none, Code); ++Index) {
        if (Code != std::errc::file_exists) {
            mContext->AddLog("Could not create Lua script: " + Code.message());

            return;
        }

        Path = mContentDirectory / (L"NewScript_" + std::to_wstring(Index) + L".lua");
    }

    mRefreshContent = true;
    mSelectedContentPath = Path;
}
