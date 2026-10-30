/**
 * @file FViewportClient.h
 * @brief 뷰포트별 입력과 장면 그리기를 구현하는 인터페이스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Engine/FHitProxy.h"

class FSceneViewport;
struct FViewportInput;
struct FVector2;

/**
 * @brief 뷰포트별 입력과 장면 그리기를 구현하는 인터페이스입니다.
 * @ingroup Engine
 */
class FViewportClient {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FViewportClient);

public:
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    virtual ~FViewportClient();

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FViewportClient);

    /**
     * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
     * @param[in] Input 현재 프레임의 뷰포트 입력 스냅샷입니다.
     * @param[in,out] Viewport 입력 좌표와 크기를 제공하는 뷰포트입니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    virtual void Input(FSceneViewport* Viewport, const FViewportInput& Input, float DeltaTime) = 0;

    /**
     * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
     * @param[in,out] Viewport 출력 타깃과 크기를 제공하는 뷰포트입니다.
     */
    virtual void Draw(FSceneViewport* Viewport) = 0;

    /**
     * @brief 뷰포트 좌표에 대응하는 선택 정보를 반환합니다.
     * @param[in] Viewport 입력 좌표와 크기를 제공하는 비소유 뷰포트입니다.
     * @param[in] Position 뷰포트 기준 마우스 픽셀 좌표입니다.
     * @return 기본 구현은 선택 대상이 없는 빈 프록시를 반환합니다.
     * @note 좌표 범위 검사는 FSceneViewport::GetHitProxy()가 수행하며 실제 판정은 파생 클라이언트가 구현합니다.
     */
    virtual FHitProxy GetHitProxy(const FSceneViewport* Viewport, const FVector2& Position) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
};
