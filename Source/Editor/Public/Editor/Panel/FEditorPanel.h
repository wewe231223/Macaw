#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class UEditorEngine;

class FEditorPanel {
public:
    ReflectTypes(FEditorPanel);

public:
    FEditorPanel(UEditorEngine& Engine, FString Name, bool Closable = false);
    virtual ~FEditorPanel();
    FEditorPanel(const FEditorPanel&) = delete;
    FEditorPanel& operator=(const FEditorPanel&) = delete;

public:
    ReflectStruct(FEditorPanel);

    void Draw(float DeltaTime);

    const FString& GetName() const;
    bool IsVisible() const;
    void SetVisible(bool Visible);

    UEditorEngine& GetEditorEngine() const;

private:
    virtual void DrawContent(float DeltaTime) = 0;

private:
    ReflectMembers();

private:
    UEditorEngine* mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mClosable{};
    ReflectProperty(mClosable, "Closable", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mVisible{true};
    ReflectProperty(mVisible, "Visible", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
