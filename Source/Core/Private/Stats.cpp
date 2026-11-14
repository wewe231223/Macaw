/**
 * @file Stats.cpp
 * @brief 프레임 통계의 등록·기록·집계와 범위 기반 시간 측정을 제공합니다.
 * @ingroup Core
 */

#include "Core/Types.h"
#include "Core/Stats.h"
#include "Core/Memory.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace Core {

    /**
     * @brief 프레임·메모리·객체 수 통계와 프레임 간격 측정 상태입니다.
     */
    namespace {
        /**
         * @brief 프레젠테이션을 포함한 프레임 작업 시간를 측정하는 통계 항목입니다.
         */
        Core::FStat FrameTime{"unit", "Frame work incl. present", Core::EStatUnit::Milliseconds, Core::EStatMode::Gauge};

        /**
         * @brief 연속 프레임 시작 사이의 밀리초 간격를 측정하는 통계 항목입니다.
         */
        Core::FStat FrameInterval{"fps", "Frame interval", Core::EStatUnit::Milliseconds, Core::EStatMode::Gauge};

        /**
         * @brief 프레임 시작 간격으로 계산한 초당 프레임 수를 측정하는 통계 항목입니다.
         */
        Core::FStat FrameRate{"fps", "FPS", Core::EStatUnit::Hertz, Core::EStatMode::Gauge};

        /**
         * @brief 현재 살아 있는 UObject 수를 측정하는 통계 항목입니다.
         */
        Core::FStat LiveObjects{"scene", "Live UObjects (global)", Core::EStatUnit::Count, Core::EStatMode::Gauge};

        /**
         * @brief 프로세스에서 생성한 UObject 누적 수를 측정하는 통계 항목입니다.
         */
        Core::FStat CreatedObjects{"scene", "Created UObjects (global)", Core::EStatUnit::Count, Core::EStatMode::Gauge};

        /**
         * @brief 추적 중인 UObject 힙 바이트 수를 측정하는 통계 항목입니다.
         */
        Core::FStat HeapBytes{"memory", "UObject heap (tracked)", Core::EStatUnit::Bytes, Core::EStatMode::Gauge};

        /**
         * @brief UObject 힙 사용량의 최대 바이트 수를 측정하는 통계 항목입니다.
         */
        Core::FStat PeakHeapBytes{"memory", "Peak UObject heap", Core::EStatUnit::Bytes, Core::EStatMode::Gauge};

        /**
         * @brief 연속 프레임의 시작 간격을 계산하기 위한 이전 시작 시각입니다.
         */
        std::chrono::steady_clock::time_point PreviousFrameStart{};
    }

    /**
     * @brief 통계 그룹 이름을 반환합니다.
     */
    const Core::FString& Core::FStatSnapshot::GetGroup() const {
        return mGroup;
    }

    /**
     * @brief 통계 항목 이름을 반환합니다.
     */
    const Core::FString& Core::FStatSnapshot::GetName() const {
        return mName;
    }

    /**
     * @brief 통계 값의 표시 단위를 반환합니다.
     */
    Core::EStatUnit Core::FStatSnapshot::GetUnit() const {
        return mUnit;
    }

    /**
     * @brief 완료된 프레임의 통계 값을 반환합니다.
     */
    double Core::FStatSnapshot::GetValue() const {
        return mValue;
    }

    /**
     * @brief 최근 최대 120프레임의 통계 평균값을 반환합니다.
     */
    double Core::FStatSnapshot::GetAverage() const {
        return mAverage;
    }

    /**
     * @brief 최근 최대 120프레임의 통계 최솟값을 반환합니다.
     */
    double Core::FStatSnapshot::GetMinimum() const {
        return mMinimum;
    }

    /**
     * @brief 최근 최대 120프레임의 통계 최댓값을 반환합니다.
     */
    double Core::FStatSnapshot::GetMaximum() const {
        return mMaximum;
    }

    /**
     * @brief 해당 프레임에 반영된 기록 횟수를 반환합니다.
     */
    Core::Uint64 Core::FStatSnapshot::GetSamples() const {
        return mSamples;
    }

    /**
     * @brief 집계에 포함된 완료 프레임 수를 반환합니다.
     */
    std::size_t Core::FStatSnapshot::GetFrameCount() const {
        return mFrameCount;
    }

    /**
     * @brief 그룹·이름·단위·모드를 설정하고 전역 통계 관리자에 등록합니다.
     */
    Core::FStat::FStat(Core::FString Group, Core::FString Name, Core::EStatUnit Unit, Core::EStatMode Mode)
		: mGroup{std::move(Group)},
		  mName{std::move(Name)},
		  mUnit{Unit},
		  mMode{Mode} {
        std::transform(mGroup.begin(), mGroup.end(), mGroup.begin(), [](unsigned char Character) {
            return static_cast<char>(std::tolower(Character));
        });

        Core::FStats::Get().Register(this);
    }

    /**
     * @brief 전역 통계 관리자에서 이 항목과 스냅샷을 등록 해제합니다.
     */
    Core::FStat::~FStat() {
        Core::FStats::Get().Unregister(this);
    }

    /**
     * @brief 유한한 값을 현재 통계에 더하고 기록 횟수를 증가시킵니다.
     */
    void Core::FStat::Add(double Value) {
        Core::FStats::Get().Record(*this, Value, false);
    }

    /**
     * @brief 현재 통계 값을 유한한 입력값으로 교체하고 기록 횟수를 증가시킵니다.
     */
    void Core::FStat::Set(double Value) {
        Core::FStats::Get().Record(*this, Value, true);
    }

    /**
     * @brief 프로세스 전역의 통계 관리자를 반환합니다.
     */
    Core::FStats& Core::FStats::Get() {
        static Core::FStats Instance{};

        return Instance;
    }

    /**
     * @brief 프레임을 시작하고 누적 통계와 기록 횟수를 초기화합니다.
     */
    bool Core::FStats::BeginFrame() {
        const std::lock_guard Lock{mMutex};

        if (mInFrame) {
            return false;
        }

        mFrameStart = std::chrono::steady_clock::now();

        const double Interval{PreviousFrameStart != std::chrono::steady_clock::time_point{} ? std::chrono::duration<double, std::milli>{mFrameStart - PreviousFrameStart}.count() : 0.0};

        PreviousFrameStart = mFrameStart;

        for (Core::FStat* Stat : mStats) {
            if (Stat->mMode == Core::EStatMode::Counter) {
                Stat->mValue = 0.0;
            }

            Stat->mSamples = 0;
        }

        FrameInterval.mValue = Interval;
        FrameInterval.mSamples = Interval > 0.0 ? 1 : 0;
        FrameRate.mValue = Interval > 0.0 ? 1000.0 / Interval : 0.0;
        FrameRate.mSamples = FrameInterval.mSamples;
        mInFrame = true;

        return true;
    }

    /**
     * @brief 프레임 값을 이력에 저장하고 정렬된 통계 스냅샷을 갱신합니다.
     */
    bool Core::FStats::EndFrame() {
        const std::lock_guard Lock{mMutex};

        if (!mInFrame) {
            return false;
        }

        FrameTime.mValue = std::chrono::duration<double, std::milli>{std::chrono::steady_clock::now() - mFrameStart}.count();
        FrameTime.mSamples = 1;

        const Core::FMemoryStats Memory{Core::FMemory::GetStats()};

        LiveObjects.mValue = static_cast<double>(Memory.mLiveObjects);
        LiveObjects.mSamples = 1;
        CreatedObjects.mValue = static_cast<double>(Memory.mCreatedObjects);
        CreatedObjects.mSamples = 1;
        HeapBytes.mValue = static_cast<double>(Memory.mHeapBytes);
        HeapBytes.mSamples = 1;
        PeakHeapBytes.mValue = static_cast<double>(Memory.mPeakHeapBytes);
        PeakHeapBytes.mSamples = 1;

        mSnapshots.clear();
        mSnapshots.reserve(mStats.size());

        for (Core::FStat* Stat : mStats) {
            Stat->mHistory[Stat->mHistoryIndex] = Stat->mValue;
            Stat->mHistoryIndex = (Stat->mHistoryIndex + 1) % Stat->mHistory.size();
            Stat->mHistoryCount = std::min(Stat->mHistoryCount + 1, Stat->mHistory.size());

            mSnapshots.push_back(MakeSnapshot(*Stat));
        }

        std::sort(mSnapshots.begin(), mSnapshots.end(), [](const Core::FStatSnapshot& First, const Core::FStatSnapshot& Second) {
            return First.GetGroup() != Second.GetGroup() ? First.GetGroup() < Second.GetGroup() : First.GetName() < Second.GetName();
        });

        ++mFrameNumber;
        mInFrame = false;

        return true;
    }

    /**
     * @brief 누적 통계·기록 횟수·이력·스냅샷과 프레임 번호를 초기화합니다.
     */
    void Core::FStats::Reset() {
        const std::lock_guard Lock{mMutex};

        for (Core::FStat* Stat : mStats) {
            if (Stat->mMode == Core::EStatMode::Counter) {
                Stat->mValue = 0.0;
            }

            Stat->mSamples = 0;
            Stat->mHistory.fill(0.0);
            Stat->mHistoryIndex = 0;
            Stat->mHistoryCount = 0;
        }

        mSnapshots.clear();
        mFrameNumber = 0;
        PreviousFrameStart = {};
    }

    /**
     * @brief 마지막 초기화 이후 완료한 통계 프레임 수를 반환합니다.
     */
    Core::Uint64 Core::FStats::GetFrameNumber() const {
        const std::lock_guard Lock{mMutex};

        return mFrameNumber;
    }

    /**
     * @brief 등록된 통계의 그룹 이름을 중복 없이 정렬하여 반환합니다.
     */
    Core::TArray<Core::FString> Core::FStats::GetGroups() const {
        const std::lock_guard Lock{mMutex};
        Core::TArray<Core::FString> Groups{};

        for (const Core::FStat* Stat : mStats) {
            if (std::find(Groups.begin(), Groups.end(), Stat->mGroup) == Groups.end()) {
                Groups.push_back(Stat->mGroup);
            }
        }

        std::sort(Groups.begin(), Groups.end());

        return Groups;
    }

    /**
     * @brief 마지막 완료 프레임의 통계 스냅샷을 그룹별로 조회합니다.
     */
    Core::TArray<Core::FStatSnapshot> Core::FStats::GetSnapshots(const Core::FString& Group) const {
        const std::lock_guard Lock{mMutex};
        Core::TArray<Core::FStatSnapshot> Result{};

        for (const Core::FStatSnapshot& Snapshot : mSnapshots) {
            if (Group.empty() || Snapshot.GetGroup() == Group) {
                Result.push_back(Snapshot);
            }
        }

        return Result;
    }

    /**
     * @brief 지정한 통계 항목의 최근 기록을 오래된 샘플부터 반환합니다.
     */
    Core::TArray<double> Core::FStats::GetHistory(std::string_view Group, std::string_view Name) const {
        const std::lock_guard Lock{mMutex};
        const auto Found{std::find_if(mStats.begin(), mStats.end(), [Group, Name](const Core::FStat* Stat) {
            return Stat->mGroup == Group && Stat->mName == Name;
        })};

        if (Found == mStats.end()) {
            return {};
        }

        const Core::FStat& Stat{**Found};
        const std::size_t Capacity{Stat.mHistory.size()};
        const Core::Index_t Oldest{static_cast<Core::Index_t>((Stat.mHistoryIndex + Capacity - Stat.mHistoryCount) % Capacity)};
        Core::TArray<double> Values{};
        Values.reserve(Stat.mHistoryCount);

        for (Core::Index_t Index{}; Index < Stat.mHistoryCount; ++Index) {
            Values.push_back(Stat.mHistory[(Oldest + Index) % Capacity]);
        }

        return Values;
    }

    /**
     * @brief 그룹과 이름을 검증한 뒤 통계 항목의 비소유 주소를 등록합니다.
     */
    void Core::FStats::Register(Core::FStat* Stat) {
        const std::lock_guard Lock{mMutex};

        if (Stat->mGroup.empty() || Stat->mName.empty() || Stat->mGroup == "all" || Stat->mGroup == "none" || Stat->mGroup == "list" || Stat->mGroup == "reset" || Stat->mGroup == "shadows" || std::any_of(Stat->mGroup.begin(), Stat->mGroup.end(), [](unsigned char Character) {
            return Character == 0 || std::isspace(Character) != 0;
        }) || std::any_of(mStats.begin(), mStats.end(), [Stat](const Core::FStat* Existing) {
            return Existing->mGroup == Stat->mGroup && Existing->mName == Stat->mName;
        })) {
            throw std::invalid_argument{"A stat requires a nonempty, nonreserved group token and a unique name within the group"};
        }

        mStats.push_back(Stat);
    }

    /**
     * @brief 통계 항목의 등록과 해당 항목의 완료 프레임 스냅샷을 제거합니다.
     */
    void Core::FStats::Unregister(Core::FStat* Stat) {
        const std::lock_guard Lock{mMutex};

        std::erase(mStats, Stat);
        std::erase_if(mSnapshots, [Stat](const Core::FStatSnapshot& Snapshot) {
            return Snapshot.GetGroup() == Stat->mGroup && Snapshot.GetName() == Stat->mName;
        });
    }

    /**
     * @brief 뮤텍스로 보호하여 유한한 값의 누적 또는 교체를 반영합니다.
     */
    void Core::FStats::Record(Core::FStat& Stat, double Value, bool Replace) {
        if (!std::isfinite(Value)) {
            return;
        }

        const std::lock_guard Lock{mMutex};
        const double Next{Replace ? Value : Stat.mValue + Value};

        if (std::isfinite(Next)) {
            Stat.mValue = Next;
            ++Stat.mSamples;
        }
    }

    /**
     * @brief 통계 항목의 현재 값과 순환 이력을 스냅샷으로 집계합니다.
     */
    Core::FStatSnapshot Core::FStats::MakeSnapshot(const Core::FStat& Stat) const {
        Core::FStatSnapshot Snapshot{};
        Snapshot.mGroup = Stat.mGroup;
        Snapshot.mName = Stat.mName;
        Snapshot.mUnit = Stat.mUnit;
        Snapshot.mValue = Stat.mValue;
        Snapshot.mSamples = Stat.mSamples;
        Snapshot.mFrameCount = Stat.mHistoryCount;

        if (Stat.mHistoryCount > 0) {
            Snapshot.mMinimum = Stat.mHistory[0];
            Snapshot.mMaximum = Stat.mHistory[0];

            for (Core::Index_t Index{}; Index < Stat.mHistoryCount; ++Index) {
                const double Value{Stat.mHistory[Index]};

                Snapshot.mAverage += Value / static_cast<double>(Stat.mHistoryCount);
                Snapshot.mMinimum = std::min(Snapshot.mMinimum, Value);
                Snapshot.mMaximum = std::max(Snapshot.mMaximum, Value);
            }
        }

        return Snapshot;
    }

    /**
     * @brief 현재 단조 시계 시각을 측정 시작점으로 저장합니다.
     */
    Core::FStopwatch::FStopwatch()
		: mStart{std::chrono::steady_clock::now()} {
    }

    /**
     * @brief 경과 시간의 기준 시각을 현재 단조 시계 시각으로 재설정합니다.
     */
    void Core::FStopwatch::Reset() {
        mStart = std::chrono::steady_clock::now();
    }

    /**
     * @brief 생성 또는 마지막 초기화 이후의 경과 시간을 반환합니다.
     */
    double Core::FStopwatch::GetElapsedMilliseconds() const {
        return std::chrono::duration<double, std::milli>{std::chrono::steady_clock::now() - mStart}.count();
    }

    /**
     * @brief 경과 시간을 누적할 항목을 연결하고 범위 시간 측정을 시작합니다.
     */
    Core::FScopedStatTimer::FScopedStatTimer(Core::FStat& Stat)
		: mStat{&Stat} {
    }

    /**
     * @brief 범위의 경과 시간을 지정한 통계 항목에 밀리초 단위로 더합니다.
     */
    Core::FScopedStatTimer::~FScopedStatTimer() {
        mStat->Add(mWatch.GetElapsedMilliseconds());
    }

    /**
     * @brief 통계 프레임 시작을 시도하고 직접 시작한 경우 종료 책임을 갖습니다.
     */
    Core::FScopedStatFrame::FScopedStatFrame()
		: mOwner{Core::FStats::Get().BeginFrame()} {
    }

    /**
     * @brief 이 범위가 직접 시작한 통계 프레임의 스냅샷을 확정합니다.
     */
    Core::FScopedStatFrame::~FScopedStatFrame() {
        if (mOwner) {
            Core::FStats::Get().EndFrame();
        }
    }

}
