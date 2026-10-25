/**
 * @file FPrimitiveDrawInterface.h
 * @brief 에디터 보조 선분과 메시를 프레임 단위로 수집합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FDrawLine.h"
#include "Engine/FDrawMesh.h"
#include "Engine/FDrawText.h"

/**
 * @brief 에디터 보조 선분과 메시를 프레임 단위로 수집합니다.
 * @ingroup Engine
 */
class FPrimitiveDrawInterface {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FPrimitiveDrawInterface);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FPrimitiveDrawInterface);

    /**
     * @brief 주어진 좌표와 색상으로 보조 선분을 프레임 목록에 추가합니다.
     * @param[in] Start 선분 시작점입니다.
     * @param[in] End 선분 끝점입니다.
     * @param[in] Color 선분에 사용할 RGB 색상입니다.
     * @param[in] DepthTest 깊이 검사를 적용할지 지정합니다.
     */
    void DrawLine(const FVector& Start, const FVector& End, const FVector& Color, bool DepthTest = false);
    /**
     * @brief 현재 프레임의 보조 선분과 메시 목록을 비웁니다.
     */
    void Clear();
    void DrawMesh(const FDrawMesh& Mesh);
    const TArray<FDrawMesh>& GetMeshes() const;
    void DrawString(const FDrawText& Text);
    const TArray<FDrawText>& GetTexts() const;

    /**
     * @brief 현재 프레임에 수집한 보조 선분을 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const TArray<FDrawLine>& GetLines() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 현재 프레임에 수집한 보조 선분입니다.
     */
    TArray<FDrawLine> mLines{};
    ReflectProperty(mLines, "Lines", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FDrawMesh> mMeshes{};
    ReflectProperty(mMeshes, "Meshes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FDrawText> mTexts{};
    ReflectProperty(mTexts, "Texts", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
