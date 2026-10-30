/**
 * @file FLineRenderData.h
 * @brief 디버그 선·격자·광선 그리기 데이터를 수집하는 컨테이너를 정의합니다.
 */

#pragma once
#include "Core/STL.h"
#include "Core/Debug/ILineDrawContext.h"

/**
 * @brief 화면 두께와 깊이 정책을 가진 월드 공간 선분 레코드입니다.
 */
struct FLineDrawData {
    /**
     * @brief 월드 공간 시작점입니다.
     */
    FVector3 mStart{};

    /**
     * @brief 월드 공간 끝점입니다.
     */
    FVector3 mEnd{};

    /**
     * @brief 선에 적용할 RGBA 색상입니다.
     */
    FVector4 mColor{};

    /**
     * @brief 픽셀 단위 선 두께입니다.
     */
    float mWidthPixels{1.0f};

    /**
     * @brief 격자 간격과 종류를 전달하는 값입니다. 0이면 일반 선이며 절댓값은 월드 공간 간격입니다. 음수는 주요 격자의 두께 보정에 사용합니다.
     */
    float mGridSpacing{};

    /**
     * @brief 장면 깊이와 비교할지 또는 깊이를 무시하고 그릴지 정하는 정책입니다.
     */
    ELineDepthMode mDepthMode{ELineDepthMode::DepthTested};
};

/**
 * @brief 유효한 선분을 순서대로 수집하는 ILineDrawContext 구현입니다.
 * @note 실제 GPU 그리기는 수행하지 않으며 호출한 값의 복사본을 보관합니다.
 */
class FLineRenderData final : public ILineDrawContext {
public:
    /**
     * @brief 격자 효과가 없는 일반 선분을 추가합니다.
     * @param Start 월드 공간 시작점입니다.
     * @param End 월드 공간 끝점입니다.
     * @param Color RGBA 선 색상입니다.
     * @param WidthPixels 픽셀 단위 선 두께입니다.
     * @param DepthMode 깊이 처리 정책입니다.
     * @note 두께가 0 이하이거나 두 점의 차이의 제곱 길이가 0 이하이면 추가하지 않습니다.
     */
    void AddLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels = 1.0f, ELineDepthMode DepthMode = ELineDepthMode::DepthTested) override;

    /**
     * @brief 격자 간격 정보를 가진 선분을 추가합니다.
     * @param Start 월드 공간 시작점입니다.
     * @param End 월드 공간 끝점입니다.
     * @param Color RGBA 선 색상입니다.
     * @param WidthPixels 픽셀 단위 선 두께입니다.
     * @param GridSpacing 0은 일반 선, 양수는 격자 간격, 음수는 주요 격자선용 간격입니다.
     * @param DepthMode 깊이 처리 정책입니다.
     * @note 두께가 0 이하이거나 선분의 제곱 길이가 0 이하이면 추가하지 않습니다.
     */
    void AddGridLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels, float GridSpacing, ELineDepthMode DepthMode);

    /**
     * @brief 방향을 정규화해 끝점을 계산한 뒤 선분을 추가합니다.
     * @param Origin 월드 공간 시작점입니다.
     * @param Direction 광선 방향입니다. 정규화는 FVector::Normalize 정책을 따릅니다.
     * @param Length 월드 공간 길이입니다.
     * @param Color RGBA 선 색상입니다.
     * @param WidthPixels 픽셀 단위 선 두께입니다.
     * @param DepthMode 깊이 처리 정책입니다.
     * @note Length가 0 이하이거나 방향의 제곱 길이가 0 이하이면 추가하지 않습니다.
     */
    void AddRay(const FVector3& Origin, const FVector3& Direction, float Length, const FVector4& Color, float WidthPixels = 1.0f, ELineDepthMode DepthMode = ELineDepthMode::DepthTested);

    /**
     * @brief 현재 수집한 선분 목록을 반환합니다.
     * @return 내부 배열의 상수 참조입니다. 추가나 Clear 이후 저장 원소의 주소는 유지되지 않을 수 있습니다.
     */
    const TArray<FLineDrawData>& GetLines() const;

    /**
     * @brief 수집한 모든 선분을 비웁니다.
     */
    void Clear();

private:
    /**
     * @brief 추가 순서의 선분 레코드 목록입니다.
     */
    TArray<FLineDrawData> mLines{};
};
