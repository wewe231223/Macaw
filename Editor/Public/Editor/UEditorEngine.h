#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <filesystem>
#include "Engine/UEngine.h"
#include "Editor/Settings/FEditorSettings.h"
#include "Editor/PIE/EPIEState.h"

class FWorldEditorContext;

class UEditorEngine final : public UEngine {
public:
    ReflectTypes(UEditorEngine, UEngine);

public:
    UEditorEngine();
    ~UEditorEngine() override;

public:
    ReflectClass(UEditorEngine);

    void Initialize() override;
    void Shutdown() override;
    void Tick(float DeltaTime) override;

    bool RequestStartPIE();
    bool RequestStopPIE();
    bool RequestPausePIE();
    bool RequestResumePIE();
    EPIEState GetPIEState() const;
    bool HasPIESession() const;
    FWorldContext* GetPIEWorldContext() const;
    bool DestroyWorldContext(FWorldContext& Context) override;
    ETickType GetWorldTickType(const FWorldContext& Context) const override;

    FWorldContext& GetEditorWorldContext();
    FWorldEditorContext& GetEditorContext();

    bool LoadStartupScene();
    bool LoadScene(const std::filesystem::path& ScenePath);
    bool SaveScene(const FString& SceneName);
    bool SaveEditorSettings(const FEditorSettings& Settings);
    const std::filesystem::path& GetScenePath() const;

private:
    void ProcessPIERequests();
    void StartPIE();
    void StopPIE();
    void ResetPIEState();
    bool LoadSceneInternal(const std::filesystem::path& ScenePath, bool BRememberScene);
    void SetScenePath(const std::filesystem::path& ScenePath);

private:
    FWorldContext* mEditorWorldContext{};
    FWorldContext* mPIEWorldContext{};
    EPIEState mPIEState{EPIEState::Stopped};
    bool mPauseRequested{};
    bool mProcessingPIE{};
    std::unique_ptr<FWorldEditorContext> mEditorContext{};
    std::filesystem::path mScenePath{};
};
