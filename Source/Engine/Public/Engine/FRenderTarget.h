#pragma once

#include "Core/Types.h"

class FRenderTarget {
public:
    virtual ~FRenderTarget();

public:
    virtual Uint32 GetWidth() const = 0;
    virtual Uint32 GetHeight() const = 0;

    virtual void* GetNativeTexture() const = 0;
};
