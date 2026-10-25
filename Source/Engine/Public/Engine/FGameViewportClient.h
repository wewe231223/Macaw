/**
 * @file FGameViewportClient.h
 * @brief 실행 월드의 활성 카메라 선택과 입력·뷰포트 갱신에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/FViewportClient.h"
#include "Engine/FWorldHandle.h"
#include "Engine/FSceneView.h"

class UEngine;

/**
 * @brief 실행 월드의 활성 카메라 선택과 입력·뷰포트 갱신를 담당합니다.
 */
class FGameViewportClient final : public FViewportClient {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FGameViewportClient, FViewportClient);

public:
    /**
     * @brief 전달받은 설정과 의존성으로 FGameViewportClient의 초기 상태를 구성합니다.
     * @param[in] Engine 월드 조회와 실행을 제공할 엔진의 비소유 주소입니다.
     */
    explicit FGameViewportClient(UEngine* Engine);

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FGameViewportClient);
    /**
     * @brief 엔진에서 월드 핸들을 검증하고 실행 뷰포트의 대상을 교체합니다.
     * @param[in] World 클라이언트에 연결할 엔진 소유 월드의 세대 핸들입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool SetWorld(FWorldHandle World);
    /**
     * @brief 클라이언트 또는 세션이 사용하는 월드의 세대 핸들을 반환합니다.
     * @return 조회한 대상의 세대 핸들이며, 대상이 없으면 무효 핸들입니다.
     */
    FWorldHandle GetWorldHandle() const;
    /**
     * @brief 실행 월드의 활성 카메라로 뷰포트 크기에 맞는 장면 뷰를 계산합니다.
     * @param[in] Size 계산할 뷰포트의 가로·세로 픽셀 크기입니다.
     * @param[out] View 계산한 카메라 뷰·투영 상태를 받을 출력입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool CalcSceneView(const FVector2& Size, FSceneView& View) const;
    void Input(FSceneViewport* Viewport, const FViewportInput& Input, float DeltaTime) override;
    void Draw(FSceneViewport* Viewport) override;

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 월드 조회와 실행을 제공하는 엔진의 비소유 주소입니다.
     */
    UEngine* mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 클라이언트가 사용하는 실행 월드의 세대 핸들입니다.
     */
    FWorldHandle mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
