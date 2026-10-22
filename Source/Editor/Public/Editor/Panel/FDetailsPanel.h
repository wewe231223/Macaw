#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"
#include "CoreUObject/Reflection/FPropertyValue.h"

class FProperty;
class UObject;

class FDetailsPanel final : public FEditorPanel {
public:
    ReflectTypes(FDetailsPanel, FEditorPanel);

public:
    explicit FDetailsPanel(UEditorEngine& Engine);
    ~FDetailsPanel() override;

public:
    ReflectStruct(FDetailsPanel);

private:
    void DrawContent(float DeltaTime) override;

    void DrawReflectedObject(UObject& Object);
    bool DrawPropertyValue(const FProperty& Property, FPropertyValue& Value);

private:
    ReflectMembers();
};
