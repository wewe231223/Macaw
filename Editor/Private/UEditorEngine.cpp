#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Base/FScopedFlag.h"
#include "Editor/UEditorEngine.h"
#include "Editor/Settings/FEditorConfigManager.h"
#include "Editor/World/FWorldEditorContext.h"
#include "Editor/UndoSystem/FUndoSystem.h"
#include "Engine/Serialization/FSceneSerializer.h"
#include "Engine/Serialization/FWorldCloner.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Core/Console/Console.h"
#include "World/UWorld.h"

UEditorEngine::UEditorEngine() {
    InitializeClass(StaticClass());
}

UEditorEngine::~UEditorEngine() {
    Shutdown();
}

void UEditorEngine::Initialize() {
    if (mEditorContext != nullptr) {
        return;
    }

    Super::Initialize();
    ReflectionModules::RegisterEditor();
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
    if (!CanChangeWorldContexts() || mProcessingPIE) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Cannot shut down the editor engine during world callbacks.");
        return;
    }

    const FScopedFlag ProcessingPIE{mProcessingPIE};

    StopPIE();
    FUndoSystem::Reset();
    mEditorContext.reset();
    mEditorWorldContext = nullptr;
    mScenePath.clear();
    Super::Shutdown();
}

void UEditorEngine::Tick(float DeltaTime) {
    if (!IsInitialized() || mProcessingPIE || !CanChangeWorldContexts()) {
        return;
    }

    const FScopedFlag ProcessingPIE{mProcessingPIE};

    ProcessPIERequests();
    Super::Tick(DeltaTime);
    ProcessPIERequests();
}

bool UEditorEngine::RequestStartPIE() {
    if (!IsInitialized() || mEditorWorldContext == nullptr || mPIEState != EPIEState::Stopped) {
        return false;
    }

    mPauseRequested = false;
    mPIEState = EPIEState::Starting;

    return true;
}

bool UEditorEngine::RequestStopPIE() {
    if (mPIEState == EPIEState::Stopped || mPIEState == EPIEState::Stopping) {
        return false;
    }

    mPIEState = EPIEState::Stopping;

    return true;
}

bool UEditorEngine::RequestPausePIE() {
    if (!HasPIESession() || mPIEState == EPIEState::Stopping || mPauseRequested) {
        return false;
    }

    mPauseRequested = true;

    return true;
}

bool UEditorEngine::RequestResumePIE() {
    if (!HasPIESession() || mPIEState == EPIEState::Stopping || !mPauseRequested) {
        return false;
    }

    mPauseRequested = false;

    return true;
}

EPIEState UEditorEngine::GetPIEState() const {
    return mPIEState;
}

bool UEditorEngine::HasPIESession() const {
    return mPIEState != EPIEState::Stopped;
}

FWorldContext* UEditorEngine::GetPIEWorldContext() const {
    return mPIEWorldContext;
}

bool UEditorEngine::DestroyWorldContext(FWorldContext& Context) {
    if (&Context == mEditorWorldContext || &Context == mPIEWorldContext) {
        return false;
    }

    return Super::DestroyWorldContext(Context);
}

ETickType UEditorEngine::GetWorldTickType(const FWorldContext& Context) const {
    return &Context == mEditorWorldContext && mPIEWorldContext != nullptr ? ETickType::None : Super::GetWorldTickType(Context);
}

void UEditorEngine::ProcessPIERequests() {
    if (mPIEState == EPIEState::Starting) {
        StartPIE();
    }

    if (mPIEState == EPIEState::Stopping || (mPIEWorldContext != nullptr && !mPIEWorldContext->GetWorld().HasBegunPlay())) {
        StopPIE();
        return;
    }

    if (mPIEWorldContext != nullptr) {
        mPIEWorldContext->GetWorld().GetTime().SetPaused(mPauseRequested);
        mPIEState = mPauseRequested ? EPIEState::Paused : EPIEState::Playing;
    }
}

void UEditorEngine::StartPIE() {
    std::unique_ptr<UWorld> World{FWorldCloner::Clone(mEditorWorldContext->GetWorld(), EWorldType::PIE)};

    if (mPIEState == EPIEState::Stopping) {
        ResetPIEState();
        return;
    }

    if (World == nullptr) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to clone the editor world for PIE.");
        ResetPIEState();
        return;
    }

    mPIEWorldContext = AdoptWorld(World);

    if (mPIEWorldContext == nullptr) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to create a PIE world context.");
        ResetPIEState();
        return;
    }

    mPIEWorldContext->GetWorld().BeginPlay();
}

void UEditorEngine::StopPIE() {
    if (mPIEWorldContext != nullptr) {
        mPIEState = EPIEState::Stopping;

        mPIEWorldContext->GetWorld().EndPlay(EEndPlayReason::EndPlayInEditor);

        if (!Super::DestroyWorldContext(*mPIEWorldContext)) {
            return;
        }
    }

    ResetPIEState();
}

void UEditorEngine::ResetPIEState() {
    mPIEWorldContext = nullptr;
    mPIEState = EPIEState::Stopped;
    mPauseRequested = false;
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
    if (mEditorWorldContext == nullptr || mEditorContext == nullptr || HasPIESession()) {
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
