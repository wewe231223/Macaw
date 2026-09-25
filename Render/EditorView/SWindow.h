#pragma once

#include "FViewportGeometry.h"

class SWindow {
public:
    virtual ~SWindow() = default;

    virtual void SetRect(const FRect& InRect);

    const FRect& GetRect() const;

protected:
    FRect mRect{};
};
