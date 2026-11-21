/**
 * @file FAssetEditorManager.cpp
 * @brief 에셋 종류별 편집기를 열고 활성 탭과 편집기 수명을 관리합니다.
 * @ingroup Editor
 */

#include "FAssetEditorManager.h"
#include "FAssetEditorToolkit.h"
#include "FMaterialEditorToolkit.h"
#include "FPhysicalMaterialEditorToolkit.h"
#include "FTextureEditorToolkit.h"
#include "FEditorWorkspace.h"
#include "FEditorCommands.h"
#include "Core/FilePath.h"
#include "Editor/FEditorContext.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FMeshViewer.h"
#include "FBlendSpaceEditor.h"
#include "FAnimGraphEditor.h"
#include "FAnimSequenceEditor.h"
#include "Engine/FProject.h"
#include "Engine/AActor.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Asset/UMaterial.h"
#include "Asset/UTexture2D.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include "RenderCore/Shader/FShaderCompiler.h"
#include "RenderCore/Shader/UShader.h"
#include "EditorStyle.h"
#include <imgui_internal.h>
#include <d3d11.h>
#include <algorithm>
#include <charconv>
#include <variant>
#include <type_traits>

using namespace Editor;

/**
 * @brief FAssetEditorManager 구현에 한정되는 내부 형식·상수·도우미 함수입니다.
 */
namespace {
    void AddShaderDiagnostic(std::string_view Line, const std::filesystem::path& SourcePath, std::vector<NativeIDE::FIDEContent::FDiagnostic>& Diagnostics) {
        const auto Suffix{Line.find("): ")};
        const auto Prefix{Suffix != std::string_view::npos ? Line.rfind('(', Suffix) : std::string_view::npos};
        if (Prefix == std::string_view::npos) {
            return;
        }

        const auto ReportedPath{Core::FilePath::ToLower(Core::FilePath::FromUtf8(Line.substr(0, Prefix)).lexically_normal())};
        if (ReportedPath.is_absolute() ? ReportedPath != SourcePath : ReportedPath.filename() != SourcePath.filename()) {
            return;
        }

        std::size_t Number{};
        const auto Parsed{std::from_chars(Line.data() + Prefix + 1, Line.data() + Suffix, Number)};
        if (Parsed.ec != std::errc{} || Number == 0) {
            return;
        }

        auto Message{Line.substr(Suffix + 3)};
        if (Message.ends_with('\r')) {
            Message.remove_suffix(1);
        }

        const bool Warning{Message.starts_with("warning ")};
        if (!Warning && !Message.starts_with("error ")) {
            return;
        }

        const auto Existing{std::ranges::find_if(Diagnostics, [Number, Warning](const auto& Diagnostic) {
            return Diagnostic.mLine == Number - 1 && Diagnostic.mWarning == Warning;
        })};
        if (Existing == Diagnostics.end()) {
            Diagnostics.push_back({Number - 1, std::string{Message}, Warning});
        } else if (Existing->mMessage.find(Message) == std::string::npos) {
            Existing->mMessage += '\n';
            Existing->mMessage += Message;
        }
    }

    std::future<NativeIDE::FIDEContent::FBuildResult> ValidateShaderSource(FEditorContext& Context, const std::filesystem::path& Path, std::string_view Source) {
        const auto* Renderer{Context.GetRenderer()};
        if (Renderer == nullptr) {
            return {};
        }

        auto Targets{Renderer->GetShaderValidationTargets(Path)};
        if (Targets.empty()) {
            return {};
        }

        return std::async(std::launch::async, [Targets = std::move(Targets), Path, Source = std::string{Source}]() {
            const RenderCore::FShaderCompiler Compiler{};
            const auto SourcePath{Core::FilePath::ToLower(Path)};
            NativeIDE::FIDEContent::FBuildResult Build{};
            Build.mSucceeded = true;

            for (const auto& Target : Targets) {
                Core::FString Message{};
                const bool Passed{Compiler.ValidateSource(Target, Path, Source, Message)};
                Build.mSucceeded &= Passed;
                Build.mOutput += Target.mEntryPoint + " [" + Target.mTargetProfile + ']';
                for (const auto& Macro : Target.mMacros) {
                    Build.mOutput += " " + Macro.mName + '=' + Macro.mValue;
                }
                Build.mOutput += Passed ? " passed\n" : " failed\n";
                if (!Message.empty()) {
                    Build.mOutput += Message;
                    if (!Message.ends_with('\n')) {
                        Build.mOutput += '\n';
                    }
                }

                std::string_view Remaining{Message};

                while (!Remaining.empty()) {
                    const auto End{Remaining.find('\n')};
                    AddShaderDiagnostic(Remaining.substr(0, End), SourcePath, Build.mDiagnostics);
                    Remaining.remove_prefix(End == std::string_view::npos ? Remaining.size() : End + 1);
                }
            }

            std::ranges::sort(Build.mDiagnostics, std::ranges::greater{}, &NativeIDE::FIDEContent::FDiagnostic::mWarning);

            return Build;
        });
    }

    /**
     * @brief 메시·애니메이션 편집기를 공통 에셋 툴킷 인터페이스에 연결합니다.
     */
    class FHostedAssetEditorToolkit final : public FAssetEditorToolkit {
    public:
        /**
         * @brief 호스트 툴킷이 소유할 수 있는 메시·애니메이션 편집기 형식 집합입니다.
         */
        using FContent = std::variant<std::unique_ptr<FMeshViewer>, std::unique_ptr<FAnimGraphEditor>, std::unique_ptr<FBlendSpaceEditor>, std::unique_ptr<FAnimSequenceEditor>>;

    public:
        /**
         * @brief FHostedAssetEditorToolkit의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Platform 창·입력·파일 대화상자를 제공하는 플랫폼 애플리케이션입니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         * @param[in] Type 편집할 에셋의 종류입니다.
         * @param[in] WindowName 탭 콘텐츠에 사용할 고유 ImGui 창 이름입니다.
         * @param[in] Content 호스트 툴킷이 소유할 메시 또는 애니메이션 편집기입니다.
         */
        FHostedAssetEditorToolkit(FEditorContext* Context, Platform::FWindowsApplication* Platform, const std::filesystem::path& Path, Asset::EAssetType Type, Core::FString WindowName, FContent Content, FMessageBox& Messages);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FHostedAssetEditorToolkit() override;

    public:
        /**
         * @brief 폐기된 에셋 경로를 참조하는 편집기 콘텐츠를 닫거나 해제합니다.
         * @param[in] Absolute 폐기 또는 경로 작업 대상의 절대 경로입니다.
         */
        void CloseRetired(const std::filesystem::path& Absolute) override;

        /**
         * @brief 호스팅한 에셋 탭을 그리고 그래프 편집기의 후속 저장을 처리합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Draw(float DeltaTime) override;

        /**
         * @brief 애니메이션 시퀀스 편집기의 미저장 변경을 확인할 항목을 반환합니다.
         */
        std::optional<FMessageBox::FSavePrompt> GetClosePrompt() override;

    private:
        bool RequestTabClose();

        /**
         * @brief 에셋 편집기의 프레임별 상태를 갱신합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void UpdateEditor(float DeltaTime) override;

        /**
         * @brief 메시 뷰어를 제외한 호스팅 편집기의 저장 단축키를 처리합니다.
         */
        void HandleCommands() override;

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        FEditorContext* mContext{};

        /**
         * @brief Win32 창·입력·파일 선택을 제공하는 플랫폼 애플리케이션입니다.
         */
        Platform::FWindowsApplication* mPlatform{};

        /**
         * @brief 호스팅한 메시 또는 애니메이션 편집기의 소유 객체입니다.
         */
        FContent mContent{};
        FMessageBox& mMessages;
    };

    FHostedAssetEditorToolkit::FHostedAssetEditorToolkit(FEditorContext* Context, Platform::FWindowsApplication* Platform, const std::filesystem::path& Path, Asset::EAssetType Type, Core::FString WindowName, FContent Content, FMessageBox& Messages)
		: FAssetEditorToolkit{Path, Type, std::move(WindowName)},
		  mContext{Context},
		  mPlatform{Platform},
		  mContent{std::move(Content)},
		  mMessages{Messages} {
        std::visit([this](auto& Content) {
            using FContentType = std::remove_reference_t<decltype(*Content)>;

            if constexpr (std::is_same_v<FContentType, FMeshViewer>) {
                Content->InitializeToolkit(GetTab(), mPlatform->GetInput());
            } else {
                Content->InitializeToolkit(GetTab());
            }
        },
            mContent);
        GetTab().SetCanCloseHandler([this]() {
            return RequestTabClose();
        });
    }

    FHostedAssetEditorToolkit::~FHostedAssetEditorToolkit() = default;

    void FHostedAssetEditorToolkit::UpdateEditor(float DeltaTime) {
        std::visit([this, DeltaTime](auto& Content) {
            Content->UpdateToolkit(DeltaTime, mContext->CanEditWorld());

            if (!Content->IsOpen()) {
                GetTab().SetVisible(false);
            }
        },
            mContent);
    }

    void FHostedAssetEditorToolkit::HandleCommands() {
        std::visit([this](auto& Content) {
            using FContentType = std::remove_reference_t<decltype(*Content)>;

            if constexpr (!std::is_same_v<FContentType, FMeshViewer>) {
                Content->HandleCommands(mContext->CanEditWorld());
            }
        },
            mContent);
    }

    void FHostedAssetEditorToolkit::Draw(float DeltaTime) {
        FAssetEditorToolkit::Draw(DeltaTime);
        std::visit([](auto& Content) {
            using FContentType = std::remove_reference_t<decltype(*Content)>;

            if constexpr (std::is_same_v<FContentType, FAnimGraphEditor>) {
                Content->FinishToolkit();
            }
        },
            mContent);
    }

    std::optional<FMessageBox::FSavePrompt> FHostedAssetEditorToolkit::GetClosePrompt() {
        if (auto* Content{std::get_if<std::unique_ptr<FAnimSequenceEditor>>(&mContent)}) {
            return (*Content)->GetClosePrompt();
        }

        return {};
    }

    bool FHostedAssetEditorToolkit::RequestTabClose() {
        auto Prompt{GetClosePrompt()};

        if (!Prompt) {
            return true;
        }

        mMessages.ConfirmSave({std::move(*Prompt)}, [this]() {
            GetTab().SetVisible(false);
        });

        return false;
    }

    void FHostedAssetEditorToolkit::CloseRetired(const std::filesystem::path& Absolute) {
        std::visit([&Absolute](auto& Content) {
            Content->CloseRetired(Absolute);
        },
            mContent);
    }
}

FAssetEditorManager::FSourceEditor::FSourceEditor(NativeIDE::EIDELanguage Language, const char* Title, const char* Id, FEditorContext* Context, FMessageBox& Messages, NativeIDE::FIDEContent::FValidator Validator, NativeIDE::FIDEContent::FBuildSavedHandler BuildSavedHandler)
	: mIDE{Language, std::move(Validator), std::move(BuildSavedHandler)},
	  mContent{Id, [this](float) {
          mIDE.Draw();

          if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::GetTopMostPopupModal() == nullptr) {
              if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
                  mIDE.Build();
              } else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) {
                  mIDE.Save();
              }
          }
      }},
	  mTab{Core::FString{Title} + "###" + Id, true, EEditorTabRole::TopLevel},
	  mTitle{Title},
	  mId{Id},
	  mContext{Context},
	  mMessages{Messages} {
    mTab.SetContent(&mContent);
    mTab.SetContentPadding({});
    mTab.SetVisible(false);
    mTab.SetCanCloseHandler([this]() {
        return RequestTabClose();
    });
    mTab.SetClosedHandler([this]() {
        mIDE.Close();
    });
}

void FAssetEditorManager::FSourceEditor::Open(const std::filesystem::path& Path, const std::filesystem::path& Workspace) {
    if (mTab.IsVisible() && (mIDE.IsDirty() || mIDE.HasUnbuiltChanges()) && Core::FilePath::ToLower(Path) == Core::FilePath::ToLower(mIDE.GetPath())) {
        mTab.Open();

        return;
    }

    if (auto Prompt{GetClosePrompt()}) {
        mMessages.ConfirmSave({std::move(*Prompt)}, [this, Path, Workspace]() {
            OpenNow(Path, Workspace);
        });

        return;
    }

    OpenNow(Path, Workspace);
}

void FAssetEditorManager::FSourceEditor::OpenNow(const std::filesystem::path& Path, const std::filesystem::path& Workspace) {
    if (!mIDE.Open(Path, Workspace)) {
        mContext->AddLog("Could not read source file " + Core::FilePath::ToGenericUtf8(Path));

        return;
    }

    mTab.SetName(Core::FString{mTitle} + " - " + Core::FilePath::ToUtf8(Path.filename()) + "###" + Core::FString{mId});
    mTab.Open();
}

void FAssetEditorManager::FSourceEditor::Draw(float DeltaTime, FEditorWorkspace& Workspace) {
    if (!mTab.IsVisible()) {
        return;
    }

    constexpr ImGuiWindowFlags Flags{ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse};
    const bool PendingChanges{mIDE.IsDirty() || mIDE.HasUnbuiltChanges()};
    mTab.SetWindowFlags(Flags | (PendingChanges ? ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None));
    Workspace.BeginTopLevelTab();
    mTab.Draw(DeltaTime);
    Workspace.EndTopLevelTab();
}

bool FAssetEditorManager::FSourceEditor::RequestTabClose() {
    if (mAllowTabClose || (!mIDE.IsDirty() && !mIDE.HasUnbuiltChanges())) {
        return true;
    }

    mMessages.ConfirmSave({MakeSavePrompt()}, [this]() {
        mAllowTabClose = true;
        mTab.SetVisible(false);
        mAllowTabClose = false;
    });

    return false;
}

FMessageBox::FSavePrompt FAssetEditorManager::FSourceEditor::MakeSavePrompt() {
    const auto Filename{Core::FilePath::ToUtf8(mIDE.GetPath().filename())};
    FMessageBox::FSavePrompt Prompt{Core::FString{mTitle}, mIDE.IsDirty() ? "Save changes to " + Filename + "?" : "Close " + Filename + "?"};

    if (mIDE.HasUnbuiltChanges()) {
        Prompt.mMessage += "\nThis file has unbuilt changes. To build and save, cancel and press F5 in the editor.";
    }

    if (mIDE.IsDirty()) {
        Prompt.mSave = [this]() {
            if (mIDE.Save()) {
                return true;
            }

            mTab.Open();

            return false;
        };
    }

    return Prompt;
}

void FAssetEditorManager::FSourceEditor::CloseRetired(const std::filesystem::path& Absolute) {
    if (!mTab.IsVisible()) {
        return;
    }

    const auto Relative{Core::FilePath::ToLower(mIDE.GetPath()).lexically_relative(Core::FilePath::ToLower(Absolute))};
    if (!Relative.empty() && *Relative.begin() != L"..") {
        mTab.SetVisible(false);
    }
}

std::optional<FMessageBox::FSavePrompt> FAssetEditorManager::FSourceEditor::GetClosePrompt() {
    if (mTab.IsVisible() && mIDE.IsDirty()) {
        return MakeSavePrompt();
    }

    return {};
}

bool FAssetEditorManager::FSourceEditor::IsFocused() const {
    return mTab.IsVisible() && mTab.IsFocused();
}

bool FAssetEditorManager::FSourceEditor::HasUnsavedSource(const std::function<bool(const std::filesystem::path&)>& Predicate) const {
    return mTab.IsVisible() && mIDE.IsDirty() && Predicate(mIDE.GetPath());
}

FAssetEditorManager::FAssetEditorManager(FEditorContext* Context, Platform::FWindowsApplication* Platform, FEditorAssetCatalog& Catalog, FAssetThumbnailCache& Thumbnails, FEditorWorkspace& Workspace, FEditorCommands& Commands, FMessageBox& Messages)
	: mContext{Context},
	  mPlatform{Platform},
	  mCatalog{Catalog},
	  mThumbnails{Thumbnails},
	  mWorkspace{Workspace},
	  mCommands{Commands},
	  mMessages{Messages},
	  mLuaEditor{NativeIDE::EIDELanguage::Lua, "Script IDE", "ScriptIDE", Context, Messages},
	  mShaderEditor{NativeIDE::EIDELanguage::Hlsl, "Shader IDE", "ShaderIDE", Context, Messages, [Context](const std::filesystem::path& Path, std::string_view Source) {
          return ValidateShaderSource(*Context, Path, Source);
      }, [Context]() {
          auto* Renderer{Context->GetRenderer()};

          return Renderer != nullptr && Renderer->ReloadChangedShaders() ? "Shaders reloaded in the viewport." : "Shader reload failed. See the Console.";
      }} {
}

FAssetEditorManager::~FAssetEditorManager() = default;

FAssetEditorToolkit* FAssetEditorManager::FindActiveEditor() const {
    FAssetEditorToolkit* Active{};

    for (const auto& Editor : mEditors) {
        FEditorTab& Tab{Editor->GetTab()};
        const ImGuiWindow* Window{ImGui::FindWindowByName(Tab.GetName().c_str())};

        if (!Tab.IsVisible() || Window == nullptr || Window->Hidden || Window->Collapsed) {
            continue;
        }

        if (Tab.IsFocused()) {
            return Editor.get();
        }

        if (Editor.get() == mActiveEditor) {
            Active = Editor.get();
        }
    }

    return Active;
}

FEditorToolkitHost* FAssetEditorManager::GetActiveToolkitHost() const {
    FAssetEditorToolkit* Editor{FindActiveEditor()};

    return Editor != nullptr ? &Editor->GetToolkitHost() : nullptr;
}

void FAssetEditorManager::BrowseActiveAsset() {
    if (const FAssetEditorToolkit* Editor{FindActiveEditor()}) {
        EditorStyle::BrowseAsset(Editor->GetAssetPath());
    }
}

void FAssetEditorManager::SetChangedHandler(std::function<void()> Handler) {
    mChanged = std::move(Handler);
}

bool FAssetEditorManager::HasOpenMaterial(const std::function<bool(const std::filesystem::path&)>& Predicate) const {
    return std::any_of(mEditors.begin(), mEditors.end(), [&Predicate](const auto& Editor) {
        return Editor->GetAssetType() == Asset::EAssetType::Material && Predicate(Editor->GetAssetPath());
    });
}

bool FAssetEditorManager::HasUnsavedSource(const std::function<bool(const std::filesystem::path&)>& Predicate) const {
    return mLuaEditor.HasUnsavedSource(Predicate) || mShaderEditor.HasUnsavedSource(Predicate);
}

bool FAssetEditorManager::IsFocused() const {
    return IsSourceEditorFocused() || std::any_of(mEditors.begin(), mEditors.end(), [](const auto& Editor) {
        return Editor->GetTab().IsFocused();
    });
}

bool FAssetEditorManager::IsSourceEditorFocused() const {
    return mLuaEditor.IsFocused() || mShaderEditor.IsFocused();
}

Core::TArray<FMessageBox::FSavePrompt> FAssetEditorManager::CollectSavePrompts() {
    Core::TArray<FMessageBox::FSavePrompt> Prompts{};

    for (FSourceEditor* Source : {&mLuaEditor, &mShaderEditor}) {
        if (auto Prompt{Source->GetClosePrompt()}) {
            Prompts.push_back(std::move(*Prompt));
        }
    }

    for (const auto& Editor : mEditors) {
        if (auto Prompt{Editor->GetClosePrompt()}) {
            Prompts.push_back(std::move(*Prompt));
        }
    }

    return Prompts;
}

void FAssetEditorManager::ConfirmClose(std::function<void()> Action) {
    mMessages.ConfirmSave(CollectSavePrompts(), std::move(Action));
}

void FAssetEditorManager::CloseRetired(const std::filesystem::path& Absolute) {
    mLuaEditor.CloseRetired(Absolute);
    mShaderEditor.CloseRetired(Absolute);

    for (auto& Editor : mEditors) {
        Editor->CloseRetired(Absolute);
    }
}

Core::FString FAssetEditorManager::GetTabId(const FAssetEditorToolkit& Toolkit) const {
    return Core::FilePath::ToGenericUtf8(Toolkit.GetAssetPath()) + "|" + std::to_string(static_cast<Uint32>(Toolkit.GetAssetType()));
}

void FAssetEditorManager::Draw(float DeltaTime) {
    mTabs.Draw(DeltaTime);

    mLuaEditor.Draw(DeltaTime, mWorkspace);
    mShaderEditor.Draw(DeltaTime, mWorkspace);

    if (mWorkspace.IsLevelEditorFocused() || IsSourceEditorFocused()) {
        mActiveEditor = nullptr;
    }

    for (const auto& Editor : mEditors) {
        if (Editor->GetTab().IsFocused()) {
            mActiveEditor = Editor.get();
        }
    }

    std::erase_if(mEditors, [this](const auto& Editor) {
        if (Editor->GetTab().IsVisible()) {
            return false;
        }

        if (mActiveEditor == Editor.get()) {
            mActiveEditor = nullptr;
        }

        mTabs.UnregisterTab(GetTabId(*Editor));
        return true;
    });
}

void FAssetEditorManager::OpenLuaScript(const std::filesystem::path& Path) {
    OpenSource(Path, mLuaEditor);
}

void FAssetEditorManager::OpenShaderSource(const std::filesystem::path& Path) {
    OpenSource(Path, mShaderEditor);
}

void FAssetEditorManager::OpenSource(const std::filesystem::path& Path, FSourceEditor& Editor) {
    Core::FString Error{};
    const auto* Assets{mContext->GetProject()->GetAssets()};
    const auto Absolute{Assets->ResolvePath(Path, Error)};

    if (Absolute.empty()) {
        mContext->AddLog("Could not open source file " + Core::FilePath::ToGenericUtf8(Path) + (Error.empty() ? "" : ": " + Error));

        return;
    }

    Editor.Open(Absolute, Assets->GetContentRoot());
}

void FAssetEditorManager::OpenContentAsset(const std::filesystem::path& Path, Asset::EAssetType Type) {
    if (Type == Asset::EAssetType::Scene) {
        mCommands.OpenLevel(Path);

        return;
    }

    for (auto& Editor : mEditors) {
        if (Editor->GetAssetPath() == Path && Editor->GetAssetType() == Type) {
            Editor->GetTab().Open();
            return;
        }
    }

    Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
    Core::FString Error{};
    if (Type == Asset::EAssetType::Shader) {
        if (const auto* Shader{Assets->LoadShader(Path, Error)}) {
            OpenShaderSource(Core::FilePath::FromUtf8(Shader->GetSourcePath()));
        } else if (!Error.empty()) {
            mContext->AddLog(Error);
        }

        return;
    }

    const Core::FString Key{Core::FilePath::ToGenericUtf8(Path)};
    std::unique_ptr<FAssetEditorToolkit> Toolkit{};
    if (Type == Asset::EAssetType::Material) {
        if (auto* Material{Assets->LoadMaterial(Path, Error)}) {
            Toolkit = std::make_unique<FMaterialEditorToolkit>(mContext, Path, Material, mCatalog, mThumbnails, mMessages);
        }
    } else if (Type == Asset::EAssetType::PhysicalMaterial) {
        if (const auto* Material{Assets->LoadPhysicalMaterial(Path, Error)}) {
            Toolkit = std::make_unique<FPhysicalMaterialEditorToolkit>(*mContext, Path, *Material, mMessages);
        }
    } else if (Type == Asset::EAssetType::Texture) {
        if (Asset::UTexture2D* Texture{Assets->LoadTexture(Path, Error)}; Texture != nullptr && Texture->CreateResources(static_cast<ID3D11Device*>(mContext->GetRenderer()->GetNativeDevice()))) {
            Toolkit = std::make_unique<FTextureEditorToolkit>(mContext, Path, Texture);
        }
    } else if (Type == Asset::EAssetType::Mesh || Type == Asset::EAssetType::SkeletalMesh) {
        auto Content{std::make_unique<FMeshViewer>(mContext->GetRenderer(), Assets)};
        Content->SetWindowId("###Mesh Viewer:" + Key);
        if (Content->Open(Path, Error)) {
            const Core::FString Name{Content->GetWindowId()};
            Toolkit = std::make_unique<FHostedAssetEditorToolkit>(mContext, mPlatform, Path, Type, Name, std::move(Content), mMessages);
        }
    } else if (Type == Asset::EAssetType::AnimGraph) {
        auto Content{std::make_unique<FAnimGraphEditor>(Assets, &mCatalog, &mThumbnails)};
        Content->SetWindowId("###AnimGraphEditor:" + Key);
        if (Content->Open(Path, Error)) {
            const Core::FString Name{Content->GetWindowId()};
            Toolkit = std::make_unique<FHostedAssetEditorToolkit>(mContext, mPlatform, Path, Type, Name, std::move(Content), mMessages);
        }
    } else if (Type == Asset::EAssetType::BlendSpace) {
        auto Content{std::make_unique<FBlendSpaceEditor>(Assets, &mCatalog, &mThumbnails)};
        Content->SetWindowId("###BlendSpaceEditor:" + Key);
        if (Content->Open(Path, Error)) {
            const Core::FString Name{Content->GetWindowId()};
            Toolkit = std::make_unique<FHostedAssetEditorToolkit>(mContext, mPlatform, Path, Type, Name, std::move(Content), mMessages);
        }
    } else if (Type == Asset::EAssetType::AnimSequence) {
        auto Content{std::make_unique<FAnimSequenceEditor>(mContext->GetRenderer(), Assets, &mCatalog, &mThumbnails, mMessages)};
        Content->SetWindowId("###AnimSequenceEditor:" + Key);
        if (Content->Open(Path, Error)) {
            Core::FString PreviewMesh{};
            Core::FString PreviewSkeleton{};
            Engine::AActor* Actor{mContext->GetSelectedActor()};
            auto* Component{Actor != nullptr ? CoreUObject::Cast<Engine::USkeletalMeshComponent>(Actor->GetRootComponent()) : nullptr};
            if (Component != nullptr && !mThumbnails.GetAnimationPreview(Path, PreviewMesh, PreviewSkeleton)) {
                Content->SetPreviewMesh(Component->GetMeshPath(), Component->GetSkeletonPath());
            }
            const Core::FString Name{Content->GetWindowId()};
            Toolkit = std::make_unique<FHostedAssetEditorToolkit>(mContext, mPlatform, Path, Type, Name, std::move(Content), mMessages);
        }
    }

    if (Toolkit != nullptr) {
        Toolkit->SetChangedHandler([this]() {
            if (mChanged) {
                mChanged();
            }
        });
        Toolkit->GetTab().Open();
        FAssetEditorToolkit* Opened{Toolkit.get()};
        mTabs.RegisterTab(GetTabId(*Toolkit), Core::FilePath::ToUtf8(Path.stem()), Toolkit->GetTab(), [this, Opened](float DeltaTime) {
            Opened->Tick(DeltaTime);
            mWorkspace.BeginTopLevelTab();
            Opened->Draw(DeltaTime);
            mWorkspace.EndTopLevelTab();
        });
        mEditors.push_back(std::move(Toolkit));
    } else if (!Error.empty()) {
        mContext->AddLog(Error);
    }
}
