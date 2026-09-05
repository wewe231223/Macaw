#pragma once

#include "Core/Base/TypeInfo.h"
#include "USceneObject.h"

class URenderableObject : public USceneObject
{
public:
	virtual ~URenderableObject() override = default;public:
	// JG_DECLARE_DERIVED_TYPEINFO(URenderableObject, UObject);
};