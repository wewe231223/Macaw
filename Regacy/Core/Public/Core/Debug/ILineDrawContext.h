/**
 * @file ILineDrawContext.h
 * @brief 깊이 처리와 픽셀 단위 두께를 지정하는 디버그 선 그리기 인터페이스를 정의한다.
 */

#pragma once
#include "Core/Common.h"
#include "Math/FMath.h"

/**
 * @brief 디버그 선을 장면의 깊이에 따라 가리거나 오버레이로 표시할지 지정한다.
 */
enum class ELineDepthMode : Uint8 {
    /**
     * @brief 장면 깊이 검사를 적용하여 가려진 선 부분을 표시하지 않는 모드.
     */
    DepthTested,
    /**
     * @brief 장면 깊이에 가리지 않고 선을 표시하는 오버레이 모드.
     */
    Overlay
};

/**
 * @brief 디버그 선의 추가를 렌더링 구현에 위임하는 인터페이스.
 * @details 좌표, 색, 화면상 두께 및 깊이 모드를 전달하며 실제 저장과 그리기는 파생 클래스가 구현한다.
 */
class ILineDrawContext {
public:
    /**
     * @brief 파생 그리기 컨텍스트를 인터페이스 포인터로 안전하게 소멸한다.
     */
    virtual ~ILineDrawContext() = default;

public:
    /**
     * @brief 두 점 사이의 디버그 선을 추가한다.
     * @param Start 선의 시작 위치.
     * @param End 선의 끝 위치. 시작 위치와 같은 좌표계를 사용한다.
     * @param Color 선의 RGBA 색.
     * @param WidthPixels 화면 픽셀 단위의 선 두께. 기본값은 1이다.
     * @param DepthMode 깊이 처리 모드. 기본값은 DepthTested이다.
     */
    virtual void AddLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels = 1.0f, ELineDepthMode DepthMode = ELineDepthMode::DepthTested) = 0;
};
