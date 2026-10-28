/**
 * @file FPrimitiveSceneProxy.h
 * @brief 컴포넌트에서 복사한 식별자, 메시 종류, 월드 행렬과 경계 상자입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FDrawText.h"
#include "Engine/FDrawSprite.h"

#include <optional>

class UStaticMesh;
class UMaterial;

/**
 * @brief 컴포넌트에서 복사한 식별자, 메시 종류, 월드 행렬과 경계 상자입니다.
 * @ingroup Engine
 */
struct FPrimitiveSceneProxy {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FPrimitiveSceneProxy);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FPrimitiveSceneProxy);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 렌더 프리미티브의 영속 식별자입니다.
     */
    FGuid mId{};
    ReflectProperty(mId, "Id", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프리미티브 메시 종류입니다.
     */
    EPrimitiveType mPrimitive{EPrimitiveType::Cube};
    ReflectProperty(mPrimitive, "Primitive", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 프리미티브의 월드 변환 행렬입니다.
     */
    FMatrix mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 축 정렬 경계 상자입니다.
     */
    FBox mBounds{};
    ReflectProperty(mBounds, "Bounds", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 렌더링할 정적 메시의 비소유 주소입니다.
     */
    UStaticMesh* mMesh{};
    ReflectProperty(mMesh, "Mesh", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief UTF-8 문자열입니다.
     */
    std::optional<FDrawText> mText{};
    ReflectProperty(mText, "Text", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 연결된 머티리얼의 비소유 주소입니다.
     */
    UMaterial* mMaterial{};
    ReflectProperty(mMaterial, "Material", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 스프라이트 전용 렌더 데이터입니다.
     */
    std::optional<FDrawSprite> mSprite{};
    ReflectProperty(mSprite, "Sprite", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 기본 UV에 곱할 셀 크기입니다.
     */
    FVector2 mUVScale{1.0f, 1.0f};
    ReflectProperty(mUVScale, "UVScale", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 기본 UV에 더할 셀 원점입니다.
     */
    FVector2 mUVOffset{};
    ReflectProperty(mUVOffset, "UVOffset", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
