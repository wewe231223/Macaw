#pragma once

#include "Editor/Panel/FEditorPanel.h"

class FPlaceActorsPanel final : public FEditorPanel {
public:
    ReflectTypes(FPlaceActorsPanel, FEditorPanel);

public:
    explicit FPlaceActorsPanel(FEditorContext* Context);
    ~FPlaceActorsPanel() override;

public:
    ReflectStruct(FPlaceActorsPanel);

private:
    ReflectMembers();
    void DrawContent(float DeltaTime) override;
    void DrawEntry(const FString& Preset, const FString& Label, const char* Components, const char* Icon);

private:
    std::array<char, 128> mSearch{};
    ReflectProperty(mSearch, "Search", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Int32 mCategory{};
    ReflectProperty(mCategory, "Category", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
