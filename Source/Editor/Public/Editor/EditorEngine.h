#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Engine.h"
#include "Editor/ViewportClient.h"
#include "Editor/Panel/FOutlinerPanel.h"
#include "Editor/Panel/FDetailsPanel.h"
#include "Editor/Panel/FConsolePanel.h"
#include "Editor/Panel/FStatisticsPanel.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "Platform/Window.h"

#include <filesystem>

class AActor;
class AStaticMeshActor;
class FRenderTarget;
enum class EPrimitiveType : Uint8;

class UEditorEngine final : public UEngine {
public:
    ReflectTypes(UEditorEngine, UEngine);

public:
    UEditorEngine();
    ~UEditorEngine() override;

public:
    ReflectClass(UEditorEngine);

    bool InitializeEditor(IRendererModule& Renderer, FWindowsApplication& Platform);
    void Shutdown() override;
    void TickFrame(float DeltaTime);

    AStaticMeshActor* SpawnPrimitive(EPrimitiveType Type);
    bool DestroySelectedActor();

    void SelectActor(FObjectHandle Handle);
    void ClearSelection();
    AActor* GetSelectedActor() const;
    UObject* GetSelectedObject() const;
    void SelectObject(FObjectHandle Handle);

    bool EditProperty(UObject& Object, const FProperty& Property, const FPropertyValue& Value, bool Interactive = false);
    bool ApplyTransform(AActor& Actor, const FTransform& Transform, bool Interactive = false);

    bool SaveScene(const std::filesystem::path& Path);
    bool LoadScene(const std::filesystem::path& Path);
    bool NewScene();

    void AddLog(const FString& Message);
    void ExecuteConsoleCommand(const FString& Command);

    FEditorViewportClient& GetViewportClient();
    FRenderTarget* GetViewportTarget() const;

    bool IsSceneDirty() const;
    bool WantsExit() const;

private:
    ReflectMembers();

    void DrawMenu();
    void DrawViewport();

    void BuildLayout();
    std::filesystem::path ChooseFile(bool Save);

private:
    FWindowsApplication* mPlatform{};
    ReflectProperty(mPlatform, "Platform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FEditorViewportClient mClient;
    ReflectProperty(mClient, "Client", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::unique_ptr<FSceneViewport> mViewport{};
    ReflectProperty(mViewport, "Viewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FOutlinerPanel mOutlinerPanel;
    ReflectProperty(mOutlinerPanel, "OutlinerPanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FDetailsPanel mDetailsPanel;
    ReflectProperty(mDetailsPanel, "DetailsPanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FConsolePanel mConsolePanel;
    ReflectProperty(mConsolePanel, "ConsolePanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FStatisticsPanel mStatisticsPanel;
    ReflectProperty(mStatisticsPanel, "StatisticsPanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FObjectHandle mSelectedActor{};
    ReflectProperty(mSelectedActor, "SelectedActor", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FObjectHandle mSelectedObject{};
    ReflectProperty(mSelectedObject, "SelectedObject", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    std::filesystem::path mScenePath{};
    ReflectProperty(mScenePath, "ScenePath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FString mIniPath{};
    ReflectProperty(mIniPath, "IniPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    float mDeltaTime{0.016f};
    ReflectProperty(mDeltaTime, "DeltaTime", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mInitialized{};
    ReflectProperty(mInitialized, "Initialized", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mContextCreated{};
    ReflectProperty(mContextCreated, "ContextCreated", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mWin32Ready{};
    ReflectProperty(mWin32Ready, "Win32Ready", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mDxReady{};
    ReflectProperty(mDxReady, "DxReady", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mDirty{};
    ReflectProperty(mDirty, "Dirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mExit{};
    ReflectProperty(mExit, "Exit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
