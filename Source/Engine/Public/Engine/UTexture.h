#pragma once

#include "CoreUObject/UObject.h"
#include "Engine/ETextureFormat.h"

class UTexture : public UObject {
public:
    ReflectTypes(UTexture, UObject);

public:
    UTexture();
    ~UTexture() override;

public:
    ReflectClass(UTexture, EClassFlags::Abstract);

    virtual Uint32 GetSizeX() const = 0;
    virtual Uint32 GetSizeY() const = 0;
    virtual ETextureFormat GetFormat() const = 0;
    virtual bool IsSRGB() const = 0;
    virtual Uint64 GetRenderRevision() const = 0;
    virtual bool IsValid() const = 0;

private:
    ReflectMembers();
};
