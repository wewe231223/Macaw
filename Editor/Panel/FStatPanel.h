#pragma once

#include "Editor/Panel/FEditorWindow.h"
#include "Editor/Panel/Stats/StatWindow.h"
#include "Core/Channel/FEditorInfo.h"
#include "../../Core/Console/Console.h"
#include "../../Core/Channel/FStateChannel.h"

class FStatPanel : public FEditorWindow {
public:
    explicit FStatPanel(UWorld& InWorld, FStateChannel<FStatDisplayFlags>::FReader InReader);

    bool CheckVisible();

private:
    void DrawContents() override;

    UWorld* mWorld{nullptr};

    FStateChannel<FStatDisplayFlags>::FReader mModeReader{};
    FStatDisplayFlags mStatFlags{};
};
