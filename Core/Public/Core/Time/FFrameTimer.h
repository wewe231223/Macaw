/**
 * @file FFrameTimer.h
 * @brief 단조 시계를 사용해 프레임 간격, 누적 시간과 업데이트용 시간 상한을 관리합니다.
 */
#pragma once

#include <chrono>
#include <cstdint>

/**
 * @brief Tick 호출 사이의 실제 시간과 상한을 적용한 업데이트 시간을 제공하는 타이머입니다.
 * @note 모든 시간 값의 단위는 초이며, 경과 시간은 마지막 Tick 시점까지의 값입니다.
 */
class FFrameTimer {
public:
    /**
     * @brief 타이머를 초기화하고 업데이트 시간 상한을 기본값 0.1초로 설정합니다.
     */
    FFrameTimer();

public:
    /**
     * @brief 시작 시점과 마지막 측정 시점을 현재 시각으로 바꾸고 측정 값과 프레임 수를 0으로 초기화합니다.
     * @note 설정된 업데이트 시간 상한은 유지합니다.
     */
    void Reset();
    /**
     * @brief 현재 시각으로 프레임 간격과 누적 경과 시간을 갱신하고 프레임 수를 증가시킵니다.
     */
    void Tick();

    /**
     * @brief 마지막 Tick에서 측정한 실제 프레임 간격을 반환합니다.
     * @return 시간 상한을 적용하지 않은 초 단위 간격입니다. Reset 직후에는 0입니다.
     */
    double GetDeltaSeconds() const;
    /**
     * @brief 업데이트용 프레임 간격을 반환합니다.
     * @return 실제 프레임 간격과 설정된 시간 상한 중 작은 값입니다.
     */
    double GetUpdateDeltaSeconds() const;
    /**
     * @brief Reset부터 마지막 Tick까지의 누적 시간을 반환합니다.
     * @return 초 단위 경과 시간입니다.
     */
    double GetElapsedSeconds() const;
    /**
     * @brief Reset 이후 Tick 호출 횟수를 반환합니다.
     * @return 측정한 프레임 수입니다.
     */
    std::uint64_t GetFrameCount() const;

    /**
     * @brief 업데이트용 시간 상한을 설정합니다.
     * @param MaxDeltaSeconds 초 단위 상한입니다. 유한한 양수만 적용하고 다른 값은 무시합니다.
     */
    void SetMaxDeltaSeconds(double MaxDeltaSeconds);
    /**
     * @brief 현재 업데이트용 시간 상한을 반환합니다.
     * @return 초 단위 상한입니다.
     */
    double GetMaxDeltaSeconds() const;

private:
    /**
     * @brief 마지막 Reset에서 기록한 단조 시계 시작 시점입니다.
     */
    std::chrono::steady_clock::time_point mStartTime{};
    /**
     * @brief 다음 프레임 간격 측정의 기준 시점입니다.
     */
    std::chrono::steady_clock::time_point mLastTickTime{};
    /**
     * @brief 마지막 Tick에서 측정한 실제 프레임 간격입니다.
     */
    double mDeltaSeconds{};
    /**
     * @brief 시작 시점부터 마지막 Tick까지의 초 단위 경과 시간입니다.
     */
    double mElapsedSeconds{};
    /**
     * @brief 업데이트용 프레임 간격의 초 단위 상한입니다.
     */
    double mMaxDeltaSeconds{0.1};
    /**
     * @brief 마지막 Reset 이후의 Tick 호출 횟수입니다.
     */
    std::uint64_t mFrameCount{};
};
