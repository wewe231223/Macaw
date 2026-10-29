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

/**
 * @brief 월드 변환을 적용한 메시의 모든 삼각형과 광선의 최근접 교차를 검사합니다.
 * @param[in] Mesh 정점·인덱스 데이터 또는 에디터 메시 요청입니다.
 * @param[in] World 메시의 월드 변환 또는 액터를 소유한 월드입니다.
 * @param[in] Ray 월드 공간 광선입니다.
 * @param[out] Distance 최근접 교차 거리의 출력입니다.
 * @return 최근접 교차를 찾으면 true이며 Distance에 월드 거리를 기록합니다.
 */
bool RaycastMesh(const FMeshData* Mesh, const FMatrix& World, const FRay& Ray, float& Distance);
