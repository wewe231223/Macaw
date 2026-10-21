#pragma once

#include "Engine/Engine.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "Platform/Window.h"

#include <filesystem>

class AActor;
class AStaticMeshActor;
class FRenderTarget;
enum class EPrimitiveType : Uint8;

class FEditorViewportClient;

class UEditorEngine final : public UEngine {
public:
    ReflectTypes(UEditorEngine, UEngine);

private:
    struct FImplementation;

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

private:
    std::unique_ptr<FImplementation> mImplementation{};
};
