/**
 * @file FGpuMeshVertex.h
 * @brief GPU 입력 레이아웃에 맞춘 24바이트 위치·법선 정점입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <array>
#include <cstddef>
#include <type_traits>

struct FMeshVertex;

/**
 * @brief GPU 입력 레이아웃에 맞춘 24바이트 위치·법선 정점입니다.
 * @ingroup Renderer
 */
struct FGpuMeshVertex {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FGpuMeshVertex);

public:
    /**
     * @brief FGpuMeshVertex의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Vertex GPU 레이아웃으로 변환할 CPU 정점입니다.
     */
    explicit FGpuMeshVertex(const FMeshVertex& Vertex);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FGpuMeshVertex);

    /**
     * @brief 정점 또는 충돌 위치입니다.
     */
    std::array<float, 3> mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 표면 또는 드래그 평면 법선입니다.
     */
    std::array<float, 3> mNormal{};
    ReflectProperty(mNormal, "Normal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

static_assert(std::is_standard_layout_v<FGpuMeshVertex> && std::is_trivially_copyable_v<FGpuMeshVertex>);
static_assert(sizeof(FGpuMeshVertex) == 24 && offsetof(FGpuMeshVertex, mPosition) == 0 && offsetof(FGpuMeshVertex, mNormal) == 12);
