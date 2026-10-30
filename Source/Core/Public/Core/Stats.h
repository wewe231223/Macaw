/**
 * @file Stats.h
 * @brief 프레임 통계의 등록·기록·집계와 범위 기반 시간 측정을 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <chrono>
#include <mutex>

/**
 * @brief 통계 값의 표시 단위를 구분합니다.
 * @ingroup Core
 */
enum class EStatUnit : Uint8 {
    /**
     * @brief 개수 단위입니다.
     */
    Count,
    /**
     * @brief 밀리초 단위입니다.
     */
    Milliseconds,
    /**
     * @brief 바이트 단위입니다.
     */
    Bytes,
    /**
     * @brief 초당 횟수 단위입니다.
     */
    Hertz,
    /**
     * @brief 백분율 단위입니다.
     */
    Percent
};

/**
 * @brief 프레임마다 초기화할 누적값과 프레임 간 유지할 측정값을 구분합니다.
 * @ingroup Core
 */
enum class EStatMode : Uint8 {
    /**
     * @brief 프레임 시작 시 값을 0으로 초기화하는 누적 통계입니다.
     */
    Counter,
    /**
     * @brief 새 기록이 없으면 이전 값을 유지하는 상태 통계입니다.
     */
    Gauge
};

/**
 * @brief 완료된 프레임의 통계 값과 최근 최대 120프레임의 집계 결과입니다.
 * @ingroup Core
 */
class FStatSnapshot {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FStatSnapshot);

public:
    /**
     * @brief FStatSnapshot의 기본 상태를 초기화합니다.
     */
    FStatSnapshot() = default;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FStatSnapshot);

    /**
     * @brief 통계 그룹 이름을 반환합니다.
     * @return 스냅샷이 소유한 그룹 이름의 참조입니다.
     */
    const FString& GetGroup() const;

    /**
     * @brief 통계 항목 이름을 반환합니다.
     * @return 스냅샷이 소유한 항목 이름의 참조입니다.
     */
    const FString& GetName() const;

    /**
     * @brief 통계 값의 표시 단위를 반환합니다.
     * @return 통계 값의 표시 단위입니다.
     */
    EStatUnit GetUnit() const;

    /**
     * @brief 완료된 프레임의 통계 값을 반환합니다.
     * @return 완료된 프레임의 통계 값입니다.
     */
    double GetValue() const;

    /**
     * @brief 최근 최대 120프레임의 통계 평균값을 반환합니다.
     * @return 이력 값의 산술평균이며 이력이 없으면 0입니다.
     */
    double GetAverage() const;

    /**
     * @brief 최근 최대 120프레임의 통계 최솟값을 반환합니다.
     * @return 이력의 최솟값이며 이력이 없으면 0입니다.
     */
    double GetMinimum() const;

    /**
     * @brief 최근 최대 120프레임의 통계 최댓값을 반환합니다.
     * @return 이력의 최댓값이며 이력이 없으면 0입니다.
     */
    double GetMaximum() const;

    /**
     * @brief 해당 프레임에 반영된 기록 횟수를 반환합니다.
     * @return 해당 프레임에 반영된 기록 횟수입니다.
     */
    Uint64 GetSamples() const;

    /**
     * @brief 집계에 포함된 완료 프레임 수를 반환합니다.
     * @return 집계에 포함된 완료 프레임 수입니다.
     */
    std::size_t GetFrameCount() const;

private:
    /**
     * @brief 통계 관리자가 항목의 현재 값·이력과 스냅샷 상태를 갱신하도록 허용합니다.
     */
    friend class FStats;

    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 통계 항목의 그룹 이름입니다.
     */
    FString mGroup{};
    ReflectProperty(mGroup, "Group", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 그룹 내 통계 항목 이름입니다.
     */
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 통계 값의 표시 단위입니다.
     */
    EStatUnit mUnit{};
    ReflectProperty(mUnit, "Unit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 완료된 프레임의 통계 값입니다.
     */
    double mValue{};
    ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 최근 최대 120프레임의 평균값입니다.
     */
    double mAverage{};
    ReflectProperty(mAverage, "Average", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 최근 최대 120프레임의 최솟값입니다.
     */
    double mMinimum{};
    ReflectProperty(mMinimum, "Minimum", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 최근 최대 120프레임의 최댓값입니다.
     */
    double mMaximum{};
    ReflectProperty(mMaximum, "Maximum", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 완료 프레임에서 유한한 값으로 반영한 기록 횟수입니다.
     */
    Uint64 mSamples{};
    ReflectProperty(mSamples, "Samples", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 집계에 포함된 완료 프레임 수입니다.
     */
    std::size_t mFrameCount{};
    ReflectProperty(mFrameCount, "FrameCount", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief 이름과 단위를 갖고 전역 통계 관리자에 수명을 등록하는 측정 항목입니다.
 * @ingroup Core
 */
class FStat {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FStat);

public:
    /**
     * @brief 그룹·이름·단위·모드를 설정하고 전역 통계 관리자에 등록합니다.
     * @param[in] Group 공백 없는 그룹 토큰이며 소문자로 정규화합니다. 예약 명령 이름은 사용할 수 없습니다.
     * @param[in] Name 그룹 안에서 중복되지 않는 비어 있지 않은 항목 이름입니다.
     * @param[in] Unit 기록하거나 표시할 값의 단위입니다.
     * @param[in] Mode 기록 모드이며 Counter는 프레임마다 초기화하고 Gauge는 유지합니다.
     * @throws std::invalid_argument 그룹 또는 이름이 잘못됐거나 같은 그룹·이름이 이미 등록되어 있습니다.
     */
    FStat(FString Group, FString Name, EStatUnit Unit = EStatUnit::Count, EStatMode Mode = EStatMode::Counter);

    /**
     * @brief 전역 통계 관리자에서 이 항목과 스냅샷을 등록 해제합니다.
     */
    ~FStat();

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 생성을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FStat(const FStat& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 대입을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FStat& operator=(const FStat& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 생성을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FStat(FStat&& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 대입을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FStat& operator=(FStat&& Other) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FStat);

    /**
     * @brief 유한한 값을 현재 통계에 더하고 기록 횟수를 증가시킵니다.
     * @param[in] Value 기록할 유한한 통계 값입니다.
     */
    void Add(double Value = 1.0);

    /**
     * @brief 현재 통계 값을 유한한 입력값으로 교체하고 기록 횟수를 증가시킵니다.
     * @param[in] Value 기록할 유한한 통계 값입니다.
     */
    void Set(double Value);

private:
    /**
     * @brief 통계 관리자가 항목의 현재 값·이력과 스냅샷 상태를 갱신하도록 허용합니다.
     */
    friend class FStats;

    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 통계 항목의 그룹 이름입니다.
     */
    FString mGroup{};
    ReflectProperty(mGroup, "Group", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 그룹 내 통계 항목 이름입니다.
     */
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 통계 값의 표시 단위입니다.
     */
    EStatUnit mUnit{};
    ReflectProperty(mUnit, "Unit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프레임 시작 시 값의 초기화 여부를 정하는 기록 모드입니다.
     */
    EStatMode mMode{};
    ReflectProperty(mMode, "Mode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 현재 프레임의 누적값 또는 최근에 기록한 유지값입니다.
     */
    double mValue{};
    ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 해당 프레임에 반영된 기록 횟수입니다.
     */
    Uint64 mSamples{};
    ReflectProperty(mSamples, "Samples", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 최근 최대 120프레임의 값을 저장하는 순환 이력입니다.
     */
    std::array<double, 120> mHistory{};
    ReflectProperty(mHistory, "History", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 다음 완료 프레임 값을 기록할 순환 이력 위치입니다.
     */
    std::size_t mHistoryIndex{};
    ReflectProperty(mHistoryIndex, "HistoryIndex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 순환 이력에 저장한 유효 프레임 수입니다.
     */
    std::size_t mHistoryCount{};
    ReflectProperty(mHistoryCount, "HistoryCount", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief 동기화된 통계 기록과 프레임 경계·이력·스냅샷을 관리합니다.
 * @ingroup Core
 */
class FStats {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FStats);

private:
    /**
     * @brief FStats의 기본 상태를 초기화합니다.
     */
    FStats() = default;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 생성을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FStats(const FStats& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 대입을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FStats& operator=(const FStats& Other) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FStats);

    /**
     * @brief 프로세스 전역의 통계 관리자를 반환합니다.
     * @return 정적 수명을 갖는 통계 관리자의 참조입니다.
     */
    static FStats& Get();

    /**
     * @brief 프레임을 시작하고 누적 통계와 기록 횟수를 초기화합니다.
     * @return 새 프레임을 시작하면 true이며 이미 진행 중이면 false입니다.
     */
    bool BeginFrame();

    /**
     * @brief 프레임 값을 이력에 저장하고 정렬된 통계 스냅샷을 갱신합니다.
     * @return 진행 중인 프레임을 종료하면 true이며 시작한 프레임이 없으면 false입니다.
     */
    bool EndFrame();

    /**
     * @brief 누적 통계·기록 횟수·이력·스냅샷과 프레임 번호를 초기화합니다.
     */
    void Reset();

    /**
     * @brief 마지막 초기화 이후 완료한 통계 프레임 수를 반환합니다.
     * @return 마지막 초기화 이후 완료한 통계 프레임 수입니다.
     */
    Uint64 GetFrameNumber() const;

    /**
     * @brief 등록된 통계의 그룹 이름을 중복 없이 정렬하여 반환합니다.
     * @return 그룹 이름 목록의 복사본입니다.
     */
    TArray<FString> GetGroups() const;

    /**
     * @brief 마지막 완료 프레임의 통계 스냅샷을 그룹별로 조회합니다.
     * @param[in] Group 조회할 그룹 이름이며 빈 문자열이면 모든 그룹을 조회합니다.
     * @return 일치하는 스냅샷의 복사본이며 그룹을 생략하면 전체 목록입니다.
     */
    TArray<FStatSnapshot> GetSnapshots(const FString& Group = {}) const;

private:
    /**
     * @brief 통계 항목이 전역 관리자의 등록·기록 함수를 호출하도록 허용합니다.
     */
    friend class FStat;

    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

    /**
     * @brief 그룹과 이름을 검증한 뒤 통계 항목의 비소유 주소를 등록합니다.
     * @param[in] Stat 기록하거나 집계할 통계 항목이며 소유권을 이전하지 않습니다.
     */
    void Register(FStat* Stat);

    /**
     * @brief 통계 항목의 등록과 해당 항목의 완료 프레임 스냅샷을 제거합니다.
     * @param[in] Stat 기록하거나 집계할 통계 항목이며 소유권을 이전하지 않습니다.
     */
    void Unregister(FStat* Stat);

    /**
     * @brief 뮤텍스로 보호하여 유한한 값의 누적 또는 교체를 반영합니다.
     * @param[in,out] Stat 기록하거나 집계할 통계 항목이며 소유권을 이전하지 않습니다.
     * @param[in] Value 기록할 유한한 통계 값입니다.
     * @param[in] Replace true이면 현재 값을 교체하고 false이면 입력값을 더합니다.
     */
    void Record(FStat& Stat, double Value, bool Replace);

    /**
     * @brief 통계 항목의 현재 값과 순환 이력을 스냅샷으로 집계합니다.
     * @param[in] Stat 기록하거나 집계할 통계 항목이며 소유권을 이전하지 않습니다.
     * @return 최근 최대 120프레임의 평균·최소·최대가 포함된 값 사본입니다.
     */
    FStatSnapshot MakeSnapshot(const FStat& Stat) const;

private:
    /**
     * @brief 통계 등록·기록·조회와 프레임 경계 처리를 보호하는 뮤텍스입니다.
     */
    mutable std::mutex mMutex{};
    ReflectProperty(mMutex, "Mutex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 현재 수명이 유지되는 등록 통계 항목의 비소유 주소 목록입니다.
     */
    TArray<FStat*> mStats{};
    ReflectProperty(mStats, "Stats", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 마지막 완료 프레임의 그룹·이름순 통계 스냅샷입니다.
     */
    TArray<FStatSnapshot> mSnapshots{};
    ReflectProperty(mSnapshots, "Snapshots", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 현재 통계 프레임을 시작한 단조 시계 시각입니다.
     */
    std::chrono::steady_clock::time_point mFrameStart{};
    ReflectProperty(mFrameStart, "FrameStart", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 마지막 초기화 이후 완료한 통계 프레임 수입니다.
     */
    Uint64 mFrameNumber{};
    ReflectProperty(mFrameNumber, "FrameNumber", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 통계 프레임을 시작한 뒤 아직 종료하지 않았는지 나타냅니다.
     */
    bool mInFrame{};
    ReflectProperty(mInFrame, "InFrame", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief 단조 시계를 사용하여 초기화 이후의 경과 시간을 측정합니다.
 * @ingroup Core
 */
class FStopwatch {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FStopwatch);

public:
    /**
     * @brief 현재 단조 시계 시각을 측정 시작점으로 저장합니다.
     */
    FStopwatch();

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FStopwatch);

    /**
     * @brief 경과 시간의 기준 시각을 현재 단조 시계 시각으로 재설정합니다.
     */
    void Reset();

    /**
     * @brief 생성 또는 마지막 초기화 이후의 경과 시간을 반환합니다.
     * @return 단조 시계로 측정한 밀리초 단위 경과 시간입니다.
     */
    double GetElapsedMilliseconds() const;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 경과 시간 측정의 기준이 되는 단조 시계 시각입니다.
     */
    std::chrono::steady_clock::time_point mStart{};
    ReflectProperty(mStart, "Start", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief 범위의 경과 시간을 지정한 통계 항목에 밀리초 단위로 누적합니다.
 * @ingroup Core
 */
class FScopedStatTimer {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FScopedStatTimer);

public:
    /**
     * @brief 경과 시간을 누적할 항목을 연결하고 범위 시간 측정을 시작합니다.
     * @param[in,out] Stat 기록하거나 집계할 통계 항목이며 소유권을 이전하지 않습니다.
     */
    explicit FScopedStatTimer(FStat& Stat);

    /**
     * @brief 범위의 경과 시간을 지정한 통계 항목에 밀리초 단위로 더합니다.
     */
    ~FScopedStatTimer();

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 생성을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FScopedStatTimer(const FScopedStatTimer& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 대입을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FScopedStatTimer& operator=(const FScopedStatTimer& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 생성을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FScopedStatTimer(FScopedStatTimer&& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 대입을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FScopedStatTimer& operator=(FScopedStatTimer&& Other) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FScopedStatTimer);

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 범위의 경과 시간을 누적할 통계 항목의 비소유 주소입니다.
     */
    FStat* mStat{};
    ReflectProperty(mStat, "Stat", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 범위 시작부터의 경과 시간을 측정하는 시계입니다.
     */
    FStopwatch mWatch{};
    ReflectProperty(mWatch, "Watch", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief 범위에서 통계 프레임을 시작하고 직접 시작한 프레임만 종료합니다.
 * @ingroup Core
 */
class FScopedStatFrame {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FScopedStatFrame);

public:
    /**
     * @brief 통계 프레임 시작을 시도하고 직접 시작한 경우 종료 책임을 갖습니다.
     */
    FScopedStatFrame();

    /**
     * @brief 이 범위가 직접 시작한 통계 프레임의 스냅샷을 확정합니다.
     */
    ~FScopedStatFrame();

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 생성을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FScopedStatFrame(const FScopedStatFrame& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 대입을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FScopedStatFrame& operator=(const FScopedStatFrame& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 생성을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FScopedStatFrame(FScopedStatFrame&& Other) = delete;

    /**
     * @brief 수명과 등록 상태의 중복 관리를 막기 위해 복사·이동 대입을 금지합니다.
     * @param[in] Other 복사 또는 이동을 금지할 상대 객체입니다.
     */
    FScopedStatFrame& operator=(FScopedStatFrame&& Other) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FScopedStatFrame);

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 이 범위가 통계 프레임을 직접 시작하여 종료 책임을 갖는지 나타냅니다.
     */
    bool mOwner{};
    ReflectProperty(mOwner, "Owner", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
