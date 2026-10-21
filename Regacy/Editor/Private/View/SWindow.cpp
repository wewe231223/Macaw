#include "pch.h"
#include "Editor/View/SWindow.h"

void SWindow::SetRect(const FRect& InRect) {
    mRect = InRect;
}

const FRect& SWindow::GetRect() const {
    return mRect;
}
