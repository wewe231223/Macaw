#pragma once

#include "Core/Base/UObject.h"
#include "FTransform.h"

class USceneObject : public UObject
{
public:
    USceneObject() = default;
    virtual ~USceneObject() override = default;

    FTransform& GetTransform();
    const FTransform& GetTransform() const;

private:
    FTransform Transform;
};