#pragma once

#include <filesystem>
#include "Engine/UEngine.h"
#include "Editor/Settings/FEditorSettings.h"
#include "Editor/UEditorEngine.generated.h"

class FWorldEditorContext;

UCLASS()
class UEditorEngine final : public UEngine {
public:
    GENERATED_TYPES(UEditorEngine)

public:
    UEditorEngine();
    ~UEditorEngine() override;

public:
    GENERATED_BODY(UEditorEngine)

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
