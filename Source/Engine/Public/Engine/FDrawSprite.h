/**
 * @file FDrawSprite.h
 * @brief 텍스처 쿼드의 크기·색상·정렬 정보를 전달합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"

class UTexture2D;

/**
 * @brief 텍스처 쿼드의 크기, 색상과 카메라 정렬 여부를 전달합니다.
 */
struct FDrawSprite {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FDrawSprite);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FDrawSprite);

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 텍스처의 비소유 주소입니다.
     */
    UTexture2D* mTexture{};
    ReflectProperty(mTexture, "Texture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 로컬 쿼드의 가로·세로 월드 크기입니다.
     */
    FVector2 mSize{2.0f, 2.0f};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 표시할 RGBA 색상입니다.
     */
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 카메라의 오른쪽·위쪽 축에 쿼드를 맞출지 여부입니다.
     */
    bool mScreenAligned{true};
    ReflectProperty(mScreenAligned, "ScreenAligned", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
