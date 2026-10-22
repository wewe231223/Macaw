#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"

class FStatisticsPanel final : public FEditorPanel {
public:
    ReflectTypes(FStatisticsPanel, FEditorPanel);

public:
    explicit FStatisticsPanel(UEditorEngine& Engine);
    ~FStatisticsPanel() override;

public:
    ReflectStruct(FStatisticsPanel);

private:
    void DrawContent(float DeltaTime) override;

private:
    ReflectMembers();
};
