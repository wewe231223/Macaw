#pragma once

#include "Core/Types.h"

struct FMeshData;
struct FMatrix;
struct FRay;
struct FVector;

bool RaycastMesh(const FMeshData* Mesh, const FMatrix& World, const FRay& Ray, float& Distance, FVector* Normal = nullptr);
