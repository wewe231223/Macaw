/**
 * @file MeshPicking.h
 * @brief 월드 공간 광선과 CPU 메시 삼각형의 교차 검사에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"

struct FMeshData;
struct FMatrix;
struct FRay;

bool RaycastMesh(const FMeshData* Mesh, const FMatrix& World, const FRay& Ray, float& Distance);
