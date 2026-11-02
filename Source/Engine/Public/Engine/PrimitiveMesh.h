/**
 * @file PrimitiveMesh.h
 * @brief Plane, Cube와 Sphere 기본 메시의 생성 및 캐시 조회를 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/EPrimitiveType.h"
#include "Asset/FMeshData.h"

namespace Engine {

    /**
     * @brief 프리미티브 종류의 표시 이름을 반환합니다.
     * @param[in] Type 기본 프리미티브 종류입니다.
     * @return 프리미티브 종류의 표시 이름입니다.
     */
    Core::FString PrimitiveName(Engine::EPrimitiveType Type);

    /**
     * @brief 기본 프리미티브의 CPU 정점, 삼각형 인덱스와 경계 상자를 생성합니다.
     * @param[in] Type 기본 프리미티브 종류입니다.
     * @return 생성한 CPU 메시 데이터입니다.
     */
    Asset::FMeshData BuildPrimitive(Engine::EPrimitiveType Type);

    /**
     * @brief 종류별로 캐시된 읽기 전용 기본 메시를 반환합니다.
     * @param[in] Type 기본 프리미티브 종류입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const Asset::FMeshData& GetPrimitiveMesh(Engine::EPrimitiveType Type);

}
