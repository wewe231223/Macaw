#pragma once
#include "Editor/Message/FEditorInfo.h"

struct ImVec2;

class FStatPanel {
public:
    explicit FStatPanel(const FStatDisplayFlags& Flags);

public:
    void Draw(const ImVec2& Min, const ImVec2& Max);

private:
    const FStatDisplayFlags& mStatDisplayFlags;
};
