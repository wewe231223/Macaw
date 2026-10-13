#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <filesystem>
#include "Engine/UEngine.h"
#include "Editor/Settings/FEditorSettings.h"

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

    FWorldContext& GetEditorWorldContext();
    FWorldEditorContext& GetEditorContext();

    bool LoadStartupScene();
    bool LoadScene(const std::filesystem::path& ScenePath);
    bool SaveScene(const FString& SceneName);
    bool SaveEditorSettings(const FEditorSettings& Settings);
    const std::filesystem::path& GetScenePath() const;

private:
    bool LoadSceneInternal(const std::filesystem::path& ScenePath, bool BRememberScene);
    void SetScenePath(const std::filesystem::path& ScenePath);

private:
    FWorldContext* mEditorWorldContext{};
    std::unique_ptr<FWorldEditorContext> mEditorContext{};
    std::filesystem::path mScenePath{};
};
