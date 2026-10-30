/**
 * @file FTiledLightGrid.h
 * @brief 16×16픽셀 타일마다 영향을 주는 광원의 인덱스 목록을 구성합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneView.h"

#include <span>

/**
 * @brief 타일 조명 셰이더에 전달할 위치·방향·색상·감쇠·그림자 정보입니다.
 * @ingroup Renderer
 */
struct FGpuLight {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FGpuLight);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FGpuLight);

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 월드 위치 XYZ와 영향 반경을 저장합니다.
     */
    FVector4 mPositionAndRadius{};
    ReflectProperty(mPositionAndRadius, "PositionAndRadius", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 방향 XYZ와 광원 종류를 저장합니다.
     */
    FVector4 mDirectionAndType{};
    ReflectProperty(mDirectionAndType, "DirectionAndType", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief RGB 광원 색상과 밝기 배율을 순서대로 저장합니다.
     */
    FVector4 mColorAndIntensity{};
    ReflectProperty(mColorAndIntensity, "ColorAndIntensity", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 감쇠 지수와 내부·외부 원뿔각의 코사인, 예약값을 저장합니다.
     */
    FVector4 mFalloffAndCones{};
    ReflectProperty(mFalloffAndCones, "FalloffAndCones", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 그림자 맵의 1 기반 시작 인덱스·개수와 예약값을 저장합니다.
     */
    FVector4 mShadowInfo{};
    ReflectProperty(mShadowInfo, "ShadowInfo", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief 16×16픽셀 타일마다 영향을 주는 광원의 인덱스 목록을 구성합니다.
 * @ingroup Renderer
 */
class FTiledLightGrid {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FTiledLightGrid);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FTiledLightGrid);

public:
    /**
     * @brief 가시 광원을 GPU 형식으로 변환하고 16×16픽셀 타일별 인덱스를 구성합니다.
     * @param[in] Lights 렌더 장면에 등록된 원본 광원 프록시 목록입니다.
     * @param[in] View 카메라 행렬·위치와 출력 크기를 가진 뷰입니다.
     * @param[in] ShadowRanges 원본 광원 순서의 그림자 맵 시작 인덱스·개수이며 빈 범위면 그림자를 연결하지 않습니다.
     */
    void Build(std::span<const FLightSceneProxy> Lights, const FSceneView& View, std::span<const std::array<Uint32, 2>> ShadowRanges = {});
    /**
     * @brief 현재 뷰의 가시 광원을 GPU 형식으로 반환합니다.
     * @return 다음 Build() 호출 전까지 유지되는 광원 배열의 참조입니다.
     */
    const TArray<FGpuLight>& GetLights() const;
    /**
     * @brief 타일마다 광원 인덱스 구간을 반환합니다.
     * @return 행 우선 타일 순서로 저장한 시작 오프셋·개수 배열의 참조입니다.
     */
    const TArray<std::array<Uint32, 2>>& GetRanges() const;
    /**
     * @brief 각 타일의 광원 인덱스를 연속 배열로 반환합니다.
     * @return 다음 Build() 호출 전까지 유지되는 GPU 광원 인덱스 배열의 참조입니다.
     */
    const TArray<Uint32>& GetIndices() const;
    /**
     * @brief 광원 타일 격자의 열 개수를 반환합니다.
     * @return 현재 뷰 폭에 필요한 16픽셀 타일 수입니다.
     */
    Uint32 GetColumns() const;
    /**
     * @brief 광원 타일 격자의 행 개수를 반환합니다.
     * @return 현재 뷰 높이에 필요한 16픽셀 타일 수입니다.
     */
    Uint32 GetRows() const;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 현재 뷰의 표시 가능한 광원을 GPU 형식으로 변환한 배열입니다.
     */
    TArray<FGpuLight> mLights{};
    ReflectProperty(mLights, "Lights", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 타일마다 광원 인덱스 배열의 시작 위치와 개수를 저장합니다.
     */
    TArray<std::array<Uint32, 2>> mRanges{};
    ReflectProperty(mRanges, "Ranges", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 타일별로 연속 저장한 GPU 광원 배열의 인덱스입니다.
     */
    TArray<Uint32> mIndices{};
    ReflectProperty(mIndices, "Indices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 16픽셀 단위 광원 타일의 열 개수입니다.
     */
    Uint32 mColumns{};
    ReflectProperty(mColumns, "Columns", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 16픽셀 단위 광원 타일의 행 개수입니다.
     */
    Uint32 mRows{};
    ReflectProperty(mRows, "Rows", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
