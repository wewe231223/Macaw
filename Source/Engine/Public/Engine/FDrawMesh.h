/**
 * @file FDrawMesh.h
 * @brief 편집기 메시의 변환, 색상과 피킹을 포함한 드로 요청에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"
#include "Engine/FHitProxy.h"
#include "Engine/EEditorMesh.h"

/**
 * @brief 변환·색상과 히트 프록시를 가진 에디터 메시 그리기 요청입니다.
 * @ingroup Engine
 */
struct FDrawMesh {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FDrawMesh);

public:
    /**
     * @brief 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 정보입니다.
     */
    ReflectStruct(FDrawMesh);

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 렌더러의 고정 GPU 리소스와 CPU 피킹 형상을 식별하며 Count는 빈 요청입니다.
     */
    EEditorMesh mMesh{EEditorMesh::Count};
    ReflectProperty(mMesh, "Mesh", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 로컬 메시 정점을 월드로 옮기는 변환 행렬입니다.
     */
    FMatrix mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 편집 메시의 RGBA 색상이며 알파 값으로 평면 핸들의 투명도를 표현합니다.
     */
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 이 메시를 클릭했을 때 반환할 대상 핸들과 조작 핸들 번호입니다.
     */
    FHitProxy mHitProxy{};
    ReflectProperty(mHitProxy, "HitProxy", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 겹치는 핸들의 CPU 선택 우선순위이며 작은 값이 우선합니다.
     */
    Uint32 mPriority{2};
    ReflectProperty(mPriority, "Priority", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 장면 깊이를 검사할지 지정하며 false이면 별도 오버레이 깊이를 사용합니다.
     */
    bool mDepthTest{};
    ReflectProperty(mDepthTest, "DepthTest", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
