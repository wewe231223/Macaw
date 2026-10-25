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

struct FDrawMesh {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FDrawMesh);

public:
    ReflectStruct(FDrawMesh);

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();

public:
    EEditorMesh mMesh{EEditorMesh::Count};
    ReflectProperty(mMesh, "Mesh", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FMatrix mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FHitProxy mHitProxy{};
    ReflectProperty(mHitProxy, "HitProxy", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mPriority{2};
    ReflectProperty(mPriority, "Priority", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mDepthTest{};
    ReflectProperty(mDepthTest, "DepthTest", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
