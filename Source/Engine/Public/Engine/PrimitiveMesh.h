#pragma once

#include "Engine/EPrimitiveType.h"
#include "Engine/FMeshData.h"

FString PrimitiveName(EPrimitiveType Type);
FMeshData BuildPrimitive(EPrimitiveType Type);
const FMeshData& GetPrimitiveMesh(EPrimitiveType Type);
