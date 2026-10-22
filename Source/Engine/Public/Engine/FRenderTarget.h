#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class FRenderTarget {
public:
    ReflectTypes(FRenderTarget);

public:
    virtual ~FRenderTarget();

public:
    ReflectStruct(FRenderTarget);

    virtual Uint32 GetWidth() const = 0;
    virtual Uint32 GetHeight() const = 0;

    virtual void* GetNativeTexture() const = 0;

private:
    ReflectMembers();
};
