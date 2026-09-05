#include "USceneObject.h"

FTransform& USceneObject::GetTransform() {
	return Transform;
}

const FTransform& USceneObject::GetTransform() const
{
	return Transform;
}