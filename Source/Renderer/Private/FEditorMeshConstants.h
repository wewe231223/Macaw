/**
 * @file FEditorMeshConstants.h
 * @brief 편집기 메시 셰이더에 전달하는 월드 행렬과 색상에 대한 선언입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include <array>
#include <cstddef>
#include <type_traits>

struct FEditorMeshConstants {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FEditorMeshConstants);

public:
    ReflectStruct(FEditorMeshConstants);

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();

public:
    FMatrix mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<float, 4> mColor{};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

static_assert(std::is_standard_layout_v<FEditorMeshConstants> && std::is_trivially_copyable_v<FEditorMeshConstants> && sizeof(FEditorMeshConstants) == 80);
static_assert(offsetof(FEditorMeshConstants, mWorld) == 0 && offsetof(FEditorMeshConstants, mColor) == 64);
