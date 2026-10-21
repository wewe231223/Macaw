#include "pch.h"
#include "Editor/Panel/FStatPanel.h"
#include "Editor/Panel/Stats/StatWindow.h"

FStatPanel::FStatPanel(const FStatDisplayFlags& Flags)
	: mStatDisplayFlags(Flags) {
}

void FStatPanel::Draw(const ImVec2& Min, const ImVec2& Max) {
    DrawStatOverlay(Min, Max, mStatDisplayFlags);
}
