#pragma once

#include "Core/Types.h"

struct FMeshData;
struct FMatrix;
struct FRay;

bool RaycastMesh(const FMeshData* Mesh, const FMatrix& World, const FRay& Ray, float& Distance);
