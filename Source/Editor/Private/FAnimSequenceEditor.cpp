/**
 * @file FAnimSequenceEditor.cpp
 * @brief 애니메이션 시퀀스의 복사본을 편집하고 독립 뷰어에서 재생하는 에디터입니다.
 * @ingroup Editor
 */

#include "FAnimSequenceEditor.h"

#include "FAnimationPreviewScene.h"
#include "FAnimSequenceViewportClient.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/FAssetManager.h"
#include "Asset/USkeleton.h"
#include "Core/FilePath.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Engine/Component/USkeletalMeshComponent.h"

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <imgui.h>
#include <imgui_internal.h>

using namespace Editor;

/**
 * @brief 애니메이션 시퀀스의 복사본을 편집하고 독립 뷰어에서 재생하는 에디터의 파일 내부 보조 구현입니다.
 */
namespace {

    /**
     * @brief 표시 이름과 무관하게 창 상태를 유지하는 ImGui 창 ID입니다.
     */
    constexpr char WindowId[]{"###AnimSequenceEditor"};

    /**
     * @brief 시퀀스의 루트 모션 추출·고정 설정을 복사합니다.
     * @param Source 복사하거나 읽을 원본 데이터입니다.
     * @param Target 복사하거나 그릴 렌더 타깃입니다.
     */
    void CopySettings(const Asset::UAnimSequence& Source, Asset::UAnimSequence& Target) {
        Target.SetRootMotionEnabled(Source.IsRootMotionEnabled());
        Target.SetForceRootLock(Source.IsRootLockForced());
        Target.SetRootMotionRootLock(Source.GetRootMotionRootLock());
        Target.SetRootMotionBone(Source.GetRootMotionBone());
    }

    /**
     * @brief 시퀀스 파일 이름을 주어진 목록 너비에 맞는 표시 문자열로 만듭니다.
     * @param Path 처리할 에셋 또는 파일 경로입니다.
     * @param Width 출력 또는 UI 영역의 너비(픽셀)입니다.
     * @return 목록에 표시할 애니메이션 이름입니다.
     */
    Core::FString GetAnimationLabel(const std::filesystem::path& Path, float Width) {
        const Core::FString Name{Core::FilePath::ToUtf8(Path.stem())};

        if (ImGui::CalcTextSize(Name.c_str()).x <= Width) {
            return Name;
        }

        const float EllipsisWidth{ImGui::CalcTextSize("...").x};
        const char* Tail{Name.c_str()};

        while (*Tail != '\0' && ImGui::CalcTextSize(Tail).x + EllipsisWidth > Width) {
            ++Tail;

            while ((static_cast<Core::Uint8>(*Tail) & 0xc0) == 0x80) {
                ++Tail;
            }
        }

        return Core::FString{"..."} + Tail;
    }
}

FAnimSequenceEditor::FAnimSequenceEditor(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets, Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails)
	: mRenderer{Renderer},
	  mAssets{Assets},
	  mCatalog{Catalog},
	  mThumbnails{Thumbnails} {
}

FAnimSequenceEditor::~FAnimSequenceEditor() = default;

bool FAnimSequenceEditor::Open(const std::filesystem::path& Path, Core::FString& Error) {
    Error.clear();

    if (mAssets == nullptr) {
        Error = "The animation viewer requires an asset manager.";

        return false;
    }

    const std::filesystem::path Relative{Core::FilePath::FromUtf8(mAssets->GetRelativePath(Path, Error))};

    if (Relative.empty()) {
        return false;
    }

    if (mOpen && Relative == mPath) {
        ImGui::SetWindowFocus(WindowId);

        return true;
    }

    if (mOpen && mDirty) {
        mPendingPath = Relative;
        mConfirmClose = true;
        ImGui::SetWindowFocus(WindowId);

        return true;
    }

    return OpenNow(Relative, Error);
}

bool FAnimSequenceEditor::OpenNow(const std::filesystem::path& Path, Core::FString& Error) {
    Asset::UAnimSequence* Source{mAssets->LoadAnimSequence(Path, Error)};

    if (Source == nullptr) {
        return false;
    }

    auto Editing{CoreUObject::NewObject<Asset::UAnimSequence>()};

    if (Editing == nullptr || !Editing->SetDataModel(Source->GetDataModel(), Error)) {
        return false;
    }

    Editing->SetName(Source->GetName());
    CopySettings(*Source, *Editing);
    const bool Playing{mPreview != nullptr && mPreview->IsPlaying()};

    if (mPreview == nullptr) {
        mPreview = std::make_unique<Editor::FAnimationPreviewScene>(mRenderer, mAssets);
        mViewport = std::make_unique<Editor::FAnimSequenceViewportClient>(mRenderer, mPreview.get());
    }

    mPreview->SetAnimation(Editing.get());
    mEditing = std::move(Editing);
    mTimeline.Reset(*mEditing);
    mSource = Source;
    mPath = Path;
    mDirty = false;
    mSourceChanged = false;
    mConfirmClose = false;
    mPendingPath.clear();
    mError.clear();
    mOpen = true;
    strncpy_s(mRootBone.data(), mRootBone.size(), mEditing->GetRootMotionBone().c_str(), _TRUNCATE);

    if (mPreview->GetComponent() == nullptr && !mMeshPath.empty()) {
        SetPreviewMesh(mMeshPath, mSkeletonPath);
    }

    if (Playing) {
        mPreview->Play();
    }

    ImGui::SetWindowFocus(WindowId);

    return true;
}

bool FAnimSequenceEditor::SetPreviewMesh(const Core::FString& MeshPath, const Core::FString& SkeletonPath) {
    mMeshPath = MeshPath;
    mSkeletonPath = SkeletonPath;

    if (mPreview == nullptr) {
        return false;
    }

    if (!mPreview->SetMesh(mMeshPath, mSkeletonPath, mError)) {
        return false;
    }

    mViewport->FrameMesh();

    return true;
}

bool FAnimSequenceEditor::Save() {
    mError.clear();
    Asset::UAnimSequence* Source{mEditing != nullptr ? mAssets->LoadAnimSequence(mPath, mError) : nullptr};

    if (Source == nullptr || Source != mSource || mSourceChanged) {
        mError = "The source animation changed. Close and reopen the viewer before saving.";

        return false;
    }

    const bool PreviousEnabled{Source->IsRootMotionEnabled()};
    const bool PreviousForced{Source->IsRootLockForced()};
    const Asset::ERootMotionRootLock PreviousLock{Source->GetRootMotionRootLock()};
    const Core::FString PreviousBone{Source->GetRootMotionBone()};

    CopySettings(*mEditing, *Source);

    if (!mAssets->SaveAnimSequence(mPath, *Source, mError)) {
        Source->SetRootMotionEnabled(PreviousEnabled);
        Source->SetForceRootLock(PreviousForced);
        Source->SetRootMotionRootLock(PreviousLock);
        Source->SetRootMotionBone(PreviousBone);

        return false;
    }

    mDirty = false;

    return true;
}

void FAnimSequenceEditor::Close() {
    if (mPreview != nullptr) {
        mPreview->Pause();
    }

    mOpen = false;
    mDirty = false;
    mConfirmClose = false;
    mPendingPath.clear();
    mPath.clear();
}

bool FAnimSequenceEditor::IsOpen() const {
    return mOpen;
}

bool FAnimSequenceEditor::IsFocused() const {
    ImGuiWindow* Window{mOpen ? ImGui::FindWindowByName(WindowId) : nullptr};
    ImGuiWindow* Focused{ImGui::GetCurrentContext()->NavWindow};

    return Window != nullptr && Focused != nullptr && ImGui::IsWindowChildOf(Focused, Window, true, false);
}

void FAnimSequenceEditor::CloseRetired(const std::filesystem::path& Path) {
    if (!mOpen || mAssets == nullptr) {
        return;
    }

    Core::FString Error{};
    const Core::FString Root{Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(mAssets->GetRelativePath(Path, Error))))};
    const auto Contains{[&](const std::filesystem::path& Target) {
        const Core::FString Relative{Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(mAssets->GetRelativePath(Target, Error))))};

        return !Root.empty() && !Target.empty() && (Root == "." || Relative == Root || Relative.starts_with(Root + "/"));
    }};

    if (Contains(mPath) || Contains(Core::FilePath::FromUtf8(mMeshPath)) || Contains(Core::FilePath::FromUtf8(mSkeletonPath))) {
        Close();
    }
}

void FAnimSequenceEditor::RefreshAssets() {
    Core::FString Error{};
    Asset::UAnimSequence* Source{mAssets->LoadAnimSequence(mPath, Error)};

    if (Source == nullptr) {
        mPreview->Pause();
        mSourceChanged = true;
        mError = Error;

        return;
    }

    if (Source != mSource) {
        if (mDirty) {
            mSourceChanged = true;
            mPreview->Pause();
        } else {
            const std::filesystem::path Path{mPath};
            OpenNow(Path, mError);
        }
    }

    Engine::USkeletalMeshComponent* Component{mPreview->GetComponent()};

    if (Component != nullptr) {
        const auto* Mesh{mAssets->LoadSkeletalMesh(Core::FilePath::FromUtf8(Component->GetMeshPath()), Error)};
        const auto* Skeleton{Component->GetSkeletonPath().empty() ? mAssets->FindLegacySkeleton(Core::FilePath::FromUtf8(Component->GetMeshPath())) : mAssets->LoadSkeleton(Core::FilePath::FromUtf8(Component->GetSkeletonPath()), Error)};

        if (Mesh != Component->GetSkeletalMesh() || Skeleton != Component->GetSkeleton()) {
            SetPreviewMesh(mMeshPath, mSkeletonPath);
        }
    }
}

void FAnimSequenceEditor::Draw(float DeltaTime, bool CanSave) {
    if (!mOpen || mEditing == nullptr) {
        mViewport.reset();
        mPreview.reset();
        mEditing.reset();
        mSource = nullptr;

        return;
    }

    RefreshAssets();
    const float Scale{ImGui::GetFontSize() / 16.0f};
    ImGui::SetNextWindowSize({1160.0f, 760.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({700.0f * Scale, 480.0f * Scale}, {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()});

    const Core::FString Title{Core::FilePath::ToUtf8(mPath.stem()) + " - Animation Sequence" + (mDirty ? " *" : "") + WindowId};
    bool Open{true};

    if (ImGui::Begin(Title.c_str(), &Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        HandleShortcuts(CanSave);

        if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) {
            mPreview->Tick(DeltaTime);
        }

        const ImVec2 Available{ImGui::GetContentRegionAvail()};
        const float TimelineHeight{ImGui::GetFrameHeightWithSpacing() * 3.0f + 100.0f * Scale};
        const float BodyHeight{std::max(Available.y - TimelineHeight, 80.0f)};

        if (ImGui::BeginTable("##AnimationLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Preview Settings", ImGuiTableColumnFlags_WidthFixed, 300.0f * Scale);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            if (ImGui::BeginChild("##AnimationViewport", {0.0f, BodyHeight}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                const ImVec2 Size{ImGui::GetContentRegionAvail()};
                mViewport->SetDisplay(mShowBones, mShowGrid, mWireframe);
                mViewport->DrawUI({Size.x, Size.y});
            }

            ImGui::EndChild();
            ImGui::TableSetColumnIndex(1);
            const float SettingsHeight{std::max(BodyHeight * 0.55f, 1.0f)};
            const float BrowserHeight{std::max(BodyHeight - SettingsHeight - ImGui::GetStyle().ItemSpacing.y, 1.0f)};

            if (ImGui::BeginChild("##AnimationSettings", {0.0f, SettingsHeight})) {
                DrawSettings(CanSave);
            }

            ImGui::EndChild();

            if (ImGui::BeginChild("##AnimationBrowser", {0.0f, BrowserHeight}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                DrawAssetBrowser();
            }

            ImGui::EndChild();
            ImGui::EndTable();
        }

        mTimeline.Draw(*mPreview, *mEditing, TimelineHeight);

        DrawClosePrompt(CanSave);
        mViewport->Render();
    } else {
        mPreview->Pause();
    }

    ImGui::End();

    if (!Open && mOpen) {
        if (mDirty) {
            mConfirmClose = true;
            mPendingPath.clear();
            ImGui::SetWindowCollapsed(WindowId, false);
        } else {
            Close();
        }
    }
}

void FAnimSequenceEditor::DrawSettings(bool CanSave) {
    ImGui::SeparatorText("Preview Settings");

    if (mCatalog != nullptr && mThumbnails != nullptr) {
        Core::FString MeshPath{mMeshPath};
        Core::FString SkeletonPath{mSkeletonPath};
        const bool MeshChanged{mPicker.Draw(*mCatalog, *mThumbnails, "Mesh", MeshPath, Asset::EAssetType::SkeletalMesh, "Select skeletal mesh")};
        const bool SkeletonChanged{mPicker.Draw(*mCatalog, *mThumbnails, "Skeleton", SkeletonPath, Asset::EAssetType::Skeleton, "Embedded skeleton")};

        if (MeshChanged || SkeletonChanged) {
            SetPreviewMesh(MeshPath, SkeletonPath);
        }
    }

    if (ImGui::Button("Frame Mesh")) {
        mViewport->FrameMesh();
    }

    ImGui::Checkbox("Show Bones", &mShowBones);
    ImGui::Checkbox("Show Grid", &mShowGrid);
    ImGui::Checkbox("Wireframe", &mWireframe);

    int Mode{mPreview->GetPreviewMode()};
    ImGui::SetNextItemWidth(-1.0f);

    if (ImGui::Combo("##RootMotionPreview", &Mode, "Original pose\0Extract in place\0Apply root motion\0")) {
        mPreview->SetPreviewMode(Mode);
    }

    ImGui::TextDisabled("Root motion resets on each loop.");
    ImGui::SeparatorText("Animation Info");

    const Asset::FAnimDataModel& Model{mEditing->GetDataModel()};

    ImGui::TextWrapped("%s", Core::FilePath::ToGenericUtf8(mPath).c_str());
    ImGui::Text("Length: %.3f s", mEditing->GetPlayLength());
    ImGui::Text("Sample rate: %.3f fps", Model.GetFrameRate().AsDecimal());
    ImGui::Text("Last frame: %u | Keys: %u", Model.GetNumberOfFrames(), Model.GetNumberOfKeys());
    ImGui::Text("Tracks: %zu", Model.GetBoneAnimationTracks().size());

    if (const Engine::USkeletalMeshComponent* Component{mPreview->GetComponent()}) {
        const auto& Bones{Component->GetSkeleton()->GetBones()};
        std::unordered_set<Core::FString> Names{};

        for (const auto& Bone : Bones) {
            Names.insert(Bone.GetName());
        }

        std::size_t Matched{};

        for (const auto& Track : Model.GetBoneAnimationTracks()) {
            Matched += Names.contains(Track.mName) ? 1 : 0;
        }

        ImGui::TextWrapped("Bones: %zu | Matched tracks: %zu / %zu", Bones.size(), Matched, Model.GetBoneAnimationTracks().size());

        if (Matched < Model.GetBoneAnimationTracks().size()) {
            ImGui::TextWrapped("Some tracks have no matching bone. Unanimated bones use the reference pose.");
        }
    }

    ImGui::SeparatorText("Root Motion Settings");
    ImGui::BeginDisabled(!CanSave || mSourceChanged);

    bool Changed{};
    bool Enabled{mEditing->IsRootMotionEnabled()};
    bool Forced{mEditing->IsRootLockForced()};
    int Lock{static_cast<int>(mEditing->GetRootMotionRootLock())};

    Changed |= ImGui::Checkbox("Enable Root Motion", &Enabled);
    ImGui::SetNextItemWidth(-1.0f);
    Changed |= ImGui::Combo("##RootLock", &Lock, "Ref Pose\0Anim First Frame\0Zero\0");
    Changed |= ImGui::Checkbox("Force Root Lock", &Forced);
    ImGui::SetNextItemWidth(-1.0f);
    Changed |= ImGui::InputTextWithHint("##RootBone", "Root bone (empty: single root)", mRootBone.data(), mRootBone.size());

    if (Changed) {
        mEditing->SetRootMotionEnabled(Enabled);
        mEditing->SetForceRootLock(Forced);
        mEditing->SetRootMotionRootLock(static_cast<Asset::ERootMotionRootLock>(Lock));
        mEditing->SetRootMotionBone(mRootBone.data());
        mDirty = true;
        mPreview->RefreshPose();
    }

    ImGui::EndDisabled();
    ImGui::TextDisabled(mDirty ? "Unsaved changes (Ctrl+S)" : "Ctrl+S: Save settings");

    if (mSourceChanged) {
        ImGui::TextWrapped("The source asset changed. Close and reopen the viewer to use the new version.");
    }

    if (!mError.empty()) {
        ImGui::TextWrapped("%s", mError.c_str());
    }
}

void FAnimSequenceEditor::DrawAssetBrowser() {
    ImGui::SeparatorText("Animations");

    if (mCatalog == nullptr) {
        ImGui::TextDisabled("No animation catalog available.");

        return;
    }

    ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x, 1.0f));
    ImGui::InputTextWithHint("##AnimationSearch", "Search name or path...", mAnimationSearch.data(), mAnimationSearch.size());
    ImGui::SameLine();

    if (ImGui::Button("X", {ImGui::GetFrameHeight(), ImGui::GetFrameHeight()})) {
        mAnimationSearch.fill('\0');
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear search");
    }

    const ImGuiTextFilter Filter{mAnimationSearch.data()};
    std::size_t Total{};
    std::size_t Matched{};

    for (const Editor::FEditorAssetEntry& Entry : mCatalog->GetEntries()) {
        if (Entry.GetType() == Asset::EAssetType::AnimSequence) {
            ++Total;
            Matched += Filter.PassFilter(Core::FilePath::ToGenericUtf8(Entry.GetPath()).c_str()) ? 1 : 0;
        }
    }

    ImGui::TextDisabled("%zu / %zu animations", Matched, Total);
    std::filesystem::path SelectedPath{};

    if (ImGui::BeginChild("##AnimationList", {}, ImGuiChildFlags_Borders)) {
        if (Matched == 0) {
            ImGui::TextDisabled("No matching animations.");
        }

        for (const Editor::FEditorAssetEntry& Entry : mCatalog->GetEntries()) {
            if (Entry.GetType() != Asset::EAssetType::AnimSequence) {
                continue;
            }

            const Core::FString Path{Core::FilePath::ToGenericUtf8(Entry.GetPath())};

            if (!Filter.PassFilter(Path.c_str())) {
                continue;
            }

            const bool Selected{Entry.GetPath() == mPath};
            const Core::FString Label{GetAnimationLabel(Entry.GetPath(), std::max(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x * 2.0f, 1.0f))};
            ImGui::PushID(Path.c_str());

            if (ImGui::Selectable(Label.c_str(), Selected) && !Selected) {
                SelectedPath = Entry.GetPath();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", Path.c_str());
            }

            ImGui::PopID();
        }
    }

    ImGui::EndChild();

    if (!SelectedPath.empty()) {
        Open(SelectedPath, mError);
    }
}

void FAnimSequenceEditor::StepFrame(Core::Int32 Direction) {
    mTimeline.StepFrame(*mPreview, *mEditing, Direction);
}

void FAnimSequenceEditor::HandleShortcuts(bool CanSave) {
    if (!IsFocused() || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) {
        return;
    }

    if (CanSave && !mSourceChanged && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) {
        Save();
    }

    if (!mPreview->IsReady() || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive() || ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyAlt) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        if (mPreview->IsPlaying()) {
            mPreview->Pause();
        } else {
            mPreview->Play();
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        StepFrame(-1);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        StepFrame(1);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        mPreview->Seek(0.0);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        mPreview->Seek(mEditing->GetPlayLength());
    }
}

void FAnimSequenceEditor::DrawClosePrompt(bool CanSave) {
    if (mConfirmClose) {
        ImGui::OpenPopup("Unsaved Animation Settings");
        mConfirmClose = false;
        mPreview->Pause();
    }

    if (ImGui::BeginPopupModal("Unsaved Animation Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Unsaved animation settings. Press Ctrl+S to save.");
        const bool Saved{CanSave && !mSourceChanged && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S) && Save()};
        const bool Discard{ImGui::Button("Discard")};
        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            mPendingPath.clear();
            ImGui::CloseCurrentPopup();
        }

        if (!mError.empty()) {
            ImGui::TextWrapped("%s", mError.c_str());
        }

        if (Saved || Discard) {
            ImGui::CloseCurrentPopup();

            if (mPendingPath.empty()) {
                Close();
            } else {
                const std::filesystem::path Path{mPendingPath};
                OpenNow(Path, mError);
            }
        }

        ImGui::EndPopup();
    }
}
