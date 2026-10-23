/**
 * @file FLineVertex.h
 * @brief GPU 입력 레이아웃에 맞춘 24바이트 위치·색상 선분 정점입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <array>
#include <cstddef>
#include <type_traits>

struct FVector;

/**
 * @brief GPU 입력 레이아웃에 맞춘 24바이트 위치·색상 선분 정점입니다.
 * @ingroup Renderer
 */
struct FLineVertex {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FLineVertex);

public:
    /**
     * @brief FLineVertex의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Position 변환하거나 투영할 위치입니다.
     * @param[in] Color 선분에 사용할 RGB 색상입니다.
     */
    FLineVertex(const FVector& Position, const FVector& Color);

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
    ReflectStruct(FLineVertex);

    /**
     * @brief 정점 또는 충돌 위치입니다.
     */
    std::array<float, 3> mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 표시 색상입니다.
     */
    std::array<float, 3> mColor{};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

static_assert(std::is_standard_layout_v<FLineVertex> && std::is_trivially_copyable_v<FLineVertex>);
static_assert(sizeof(FLineVertex) == 24 && offsetof(FLineVertex, mPosition) == 0 && offsetof(FLineVertex, mColor) == 12);
