/**
 * @file FViewportStatistics.h
 * @brief 프레임 시간, 객체, 메모리와 렌더링 통계를 표시합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class UWorld;
struct FRenderStats;
struct FVector2;
struct ImDrawList;

/**
 * @brief 프레임 시간, 객체, 메모리와 렌더링 통계를 표시합니다.
 * @ingroup Editor
 */
class FViewportStatistics {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FViewportStatistics);
    /**
     * @brief 뷰포트 통계의 표시 그룹을 비트 플래그로 구분합니다.
     */
    enum class EGroup : Uint8 {
        /**
         * @brief 프레임 속도와 프레임 시간 통계입니다.
         */
        Fps = 1,
        /**
         * @brief 월드 액터 수와 객체 수명 통계입니다.
         */
        Scene = 2,
        /**
         * @brief CPU 및 GPU 메모리 사용량 통계입니다.
         */
        Memory = 4,
        /**
         * @brief 렌더 프리미티브·드로 호출·변환 업로드 통계입니다.
         */
        Render = 8,
        /**
         * @brief 지원하는 모든 통계 그룹의 조합입니다.
         */
        All = 15
    };

public:
    /**
     * @brief FViewportStatistics의 초기 표시 상태를 구성합니다.
     */
    FViewportStatistics() = default;
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FViewportStatistics() = default;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FViewportStatistics);
    bool ExecuteCommand(const FString& Command, FString& Output);
    bool IsEnabled(EGroup Group) const;
    /**
     * @brief 하나 이상의 뷰포트 통계 그룹이 표시되도록 설정됐는지 반환합니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    bool IsVisible() const;

    /**
     * @brief 뷰포트 이미지 위에 입력을 받지 않는 통계 내용을 그립니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    /**
     * @param[in] World 액터 수를 조회할 월드의 비소유 주소입니다.
     * @param[in] Render 이번 프레임의 렌더링 통계입니다.
     * @param[in] Origin 뷰포트 이미지의 화면상 좌측 상단 좌표입니다.
     * @param[in] Size 뷰포트 이미지의 가로·세로 픽셀 크기입니다.
     */
    void Draw(float DeltaTime, const UWorld* World, const FRenderStats& Render, const FVector2& Origin, const FVector2& Size, ImDrawList* DrawList = nullptr) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 현재 표시하도록 설정된 통계 그룹의 비트 조합입니다.
     */
    Uint8 mEnabledGroups{};
    ReflectProperty(mEnabledGroups, "EnabledGroups", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
