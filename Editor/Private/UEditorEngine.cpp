#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "Editor/UEditorEngine.h"
#include "Editor/Settings/FEditorConfigManager.h"
#include "Editor/World/FWorldEditorContext.h"
#include "Editor/UndoSystem/FUndoSystem.h"
#include "Engine/Serialization/FSceneSerializer.h"
#include "CoreUObject/TypeRegistry.h"
#include "Core/Console/Console.h"

UEditorEngine::UEditorEngine() = default;

UEditorEngine::~UEditorEngine() {
    Shutdown();
}

void UEditorEngine::Initialize() {
    if (mEditorContext != nullptr) {
        return;
    }

    UEngine::Initialize();
    TypeRegistry::Register(UEditorEngine::StaticTypeInfo());
    mEditorWorldContext = &CreateWorldContext(EWorldType::Editor);
    mEditorContext = std::make_unique<FWorldEditorContext>();

    FEditorSettings Settings{};

    FEditorConfigManager::Load(Settings);
    mEditorContext->SetEditorSettings(Settings);
    mEditorContext->SetWorld(&mEditorWorldContext->GetWorld());
    mEditorContext->InitializeChannels(*this);
    FUndoSystem::Reset();
}

void UEditorEngine::Shutdown() {
    FUndoSystem::Reset();
    mEditorContext.reset();
    mEditorWorldContext = nullptr;
    mScenePath.clear();
    UEngine::Shutdown();
}

FWorldContext& UEditorEngine::GetEditorWorldContext() {
    if (mEditorWorldContext == nullptr) {
        ErrorHandler::Report("UEditorEngine", "Editor engine is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    return *mEditorWorldContext;
}

FWorldEditorContext& UEditorEngine::GetEditorContext() {
    if (mEditorContext == nullptr) {
        ErrorHandler::Report("UEditorEngine", "Editor engine is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    return *mEditorContext;
}

bool UEditorEngine::LoadStartupScene() {
    if (mEditorContext == nullptr) {
        return false;
    }

    const FString LastScenePath{mEditorContext->GetEditorSettings().mLastLoadedScenePath};
    const std::filesystem::path ScenePath{LastScenePath.empty() ? "scenes/Default.json" : LastScenePath.c_str()};

    return LoadSceneInternal(ScenePath, !LastScenePath.empty());
}

bool UEditorEngine::LoadScene(const std::filesystem::path& ScenePath) {
    return LoadSceneInternal(ScenePath, true);
}

bool UEditorEngine::LoadSceneInternal(const std::filesystem::path& ScenePath, bool BRememberScene) {
    if (mEditorWorldContext == nullptr || mEditorContext == nullptr) {
        return false;
    }

    if (!FSceneSerializer::Load(mEditorWorldContext->GetWorld(), ScenePath)) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to load scene: %s", ScenePath.generic_string().c_str());
        return false;
    }

    mEditorContext->ClearSelection();
    FUndoSystem::Reset();

    if (BRememberScene) {
        SetScenePath(ScenePath);
    } else {
        mScenePath.clear();
    }

    Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Loaded scene: %s", ScenePath.generic_string().c_str());

    return true;
}

bool UEditorEngine::SaveScene(const FString& SceneName) {
    if (mEditorWorldContext == nullptr || mEditorContext == nullptr) {
        return false;
    }

    const std::filesystem::path Name{SceneName.c_str()};

    if (Name.empty() || Name.has_parent_path() || Name == "." || Name == ".." || SceneName.find_first_of("<>:\"/\\|?*") != FString::npos) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Invalid scene name: %s", SceneName.c_str());
        return false;
    }

    std::filesystem::path ScenePath{"scenes"};

    ScenePath /= Name;

    if (ScenePath.extension() != ".json") {
        ScenePath += ".json";
    }

    if (!mScenePath.empty() && mScenePath.extension() == ".json" && mScenePath.stem() == ScenePath.stem()) {
        ScenePath = mScenePath;
    }

    if (!FSceneSerializer::Save(mEditorWorldContext->GetWorld(), ScenePath)) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to save scene: %s", ScenePath.generic_string().c_str());
        return false;
    }

    SetScenePath(ScenePath);
    Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Saved scene: %s", mScenePath.generic_string().c_str());

    return true;
}

bool UEditorEngine::SaveEditorSettings(const FEditorSettings& Settings) {
    if (mEditorContext == nullptr) {
        return false;
    }

    FEditorSettings SavedSettings{Settings};

    SavedSettings.mLastLoadedScenePath = mScenePath.generic_string().c_str();
    mEditorContext->SetEditorSettings(SavedSettings);

    if (!FEditorConfigManager::Save(SavedSettings)) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Failed to save editor settings.");
        return false;
    }

    return true;
}

const std::filesystem::path& UEditorEngine::GetScenePath() const {
    return mScenePath;
}

void UEditorEngine::SetScenePath(const std::filesystem::path& ScenePath) {
    mScenePath = ScenePath.lexically_normal();

    FEditorSettings Settings{mEditorContext->GetEditorSettings()};

    Settings.mLastLoadedScenePath = mScenePath.generic_string().c_str();
    mEditorContext->SetEditorSettings(Settings);
    SaveEditorSettings(Settings);
}
