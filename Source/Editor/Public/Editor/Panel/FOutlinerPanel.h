#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"

class FOutlinerPanel final : public FEditorPanel {
public:
    ReflectTypes(FOutlinerPanel, FEditorPanel);

public:
    explicit FOutlinerPanel(UEditorEngine& Engine);
    ~FOutlinerPanel() override;

public:
    ReflectStruct(FOutlinerPanel);

private:
    void DrawContent(float DeltaTime) override;

private:
    ReflectMembers();
};
