/**
 * @file FMeshData.h
 * @brief CPU 메시의 위치·법선 정점, 인덱스와 로컬 경계 상자입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FMeshVertex.h"
#include "Core/Math/FBox.h"

/**
 * @brief CPU 메시의 위치·법선 정점, 인덱스와 로컬 경계 상자입니다.
 * @ingroup Engine
 */
struct FMeshData {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FMeshData);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FMeshData);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief CPU 위치·법선 정점 목록입니다.
     */
    TArray<FMeshVertex> mVertices{};
    ReflectProperty(mVertices, "Vertices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 삼각형을 구성하는 정점 인덱스 목록입니다.
     */
    TArray<Uint32> mIndices{};
    ReflectProperty(mIndices, "Indices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 축 정렬 경계 상자입니다.
     */
    FBox mBounds{};
    ReflectProperty(mBounds, "Bounds", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
