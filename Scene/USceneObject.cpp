#include "PCH.h"
#include "USceneObject.h"
#include "Core/Base/UObject.h"
#include "Core/Base/FTransform.h" 

FTransform& USceneObject::GetTransform() 
{
    return Transform;
}

const FTransform& USceneObject::GetTransform() const
{
    return Transform;
}