#pragma once

#include "Core/Base/UObject.h"
#include "Core/Base/FTransform.h"
#include "Core/Base/TypeInfo.h"

class USceneObject : public UObject
{
public:
    USceneObject() = default;
    virtual ~USceneObject() override = default;

    FTransform& GetTransform();
    const FTransform& GetTransform() const;

    // JG_DECLARE_DERIVED_TYPEINFO(USceneObject, UObject)

private:
    FTransform Transform;
};