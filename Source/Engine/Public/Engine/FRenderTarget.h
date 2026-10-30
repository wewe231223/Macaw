/**
 * @file FRenderTarget.h
 * @brief 렌더 타깃의 크기와 UI 표시용 네이티브 리소스를 노출합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

/**
 * @brief 렌더 타깃의 크기와 UI 표시용 네이티브 리소스를 노출합니다.
 * @ingroup Engine
 */
class FRenderTarget {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FRenderTarget);

public:
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    virtual ~FRenderTarget();

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FRenderTarget);

    /**
     * @brief 가로 픽셀 수를 반환합니다.
     * @return 가로 픽셀 수입니다.
     */
    virtual Uint32 GetWidth() const = 0;

    /**
     * @brief 세로 픽셀 수를 반환합니다.
     * @return 세로 픽셀 수입니다.
     */
    virtual Uint32 GetHeight() const = 0;

    /**
     * @brief UI에서 샘플링할 셰이더 리소스 뷰의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    virtual void* GetNativeTexture() const = 0;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
};
