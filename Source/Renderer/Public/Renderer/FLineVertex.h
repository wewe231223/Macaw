/**
 * @file FLineVertex.h
 * @brief 픽셀 두께를 가진 선분의 GPU 인스턴스 형식을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Math/FVector4.h"

#include <type_traits>

struct FDrawLine;

/**
 * @brief 한 선분을 화면상의 두 삼각형으로 확장할 48바이트 인스턴스 데이터입니다.
 */
struct FLineVertex {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FLineVertex);

    /**
     * @brief 선분의 월드 끝점·픽셀 두께·색상을 GPU 인스턴스로 변환합니다.
     * @param[in] Line 변환할 선분의 끝점·색상·두께입니다.
     */
    explicit FLineVertex(const FDrawLine& Line);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FLineVertex);

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 시작점 XYZ와 픽셀 두께 W입니다.
     */
    FVector4 mStartAndWidth{};
    ReflectProperty(mStartAndWidth, "StartAndWidth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 선분의 끝점입니다.
     */
    FVector4 mEnd{};
    ReflectProperty(mEnd, "End", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 표시할 RGBA 색상입니다.
     */
    FVector4 mColor{};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

static_assert(std::is_standard_layout_v<FLineVertex> && std::is_trivially_copyable_v<FLineVertex> && sizeof(FLineVertex) == 48);
