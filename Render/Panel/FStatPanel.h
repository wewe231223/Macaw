#pragma once

#include "Render/Panel/FEditorWindow.h"
#include "Render/Panel/Stats/StatWindow.h"
#include "FEditorInfo.h"
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
