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
    enum class EGroup : Uint8 {
        Fps = 1,
        Scene = 2,
        Memory = 4,
        Render = 8,
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
    bool IsVisible() const;

    /**
     * @brief 뷰포트 이미지 위에 입력을 받지 않는 통계 내용을 그립니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    void Draw(float DeltaTime, const UWorld* World, const FRenderStats& Render, const FVector2& Origin, const FVector2& Size) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    Uint8 mEnabledGroups{};
    ReflectProperty(mEnabledGroups, "EnabledGroups", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
