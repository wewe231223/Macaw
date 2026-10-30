/**
 * @file Stats.cpp
 * @brief 프레임 통계의 등록·기록·집계와 범위 기반 시간 측정을 제공합니다.
 * @ingroup Core
 */

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

    const Core::FString& Core::FStatSnapshot::GetGroup() const {
        return mGroup;
    }

    const Core::FString& Core::FStatSnapshot::GetName() const {
        return mName;
    }

    Core::EStatUnit Core::FStatSnapshot::GetUnit() const {
        return mUnit;
    }

    double Core::FStatSnapshot::GetValue() const {
        return mValue;
    }

    double Core::FStatSnapshot::GetAverage() const {
        return mAverage;
    }

    double Core::FStatSnapshot::GetMinimum() const {
        return mMinimum;
    }

    double Core::FStatSnapshot::GetMaximum() const {
        return mMaximum;
    }

    Core::Uint64 Core::FStatSnapshot::GetSamples() const {
        return mSamples;
    }

    std::size_t Core::FStatSnapshot::GetFrameCount() const {
        return mFrameCount;
    }

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

    Core::FStat::~FStat() {
        Core::FStats::Get().Unregister(this);
    }

    void Core::FStat::Add(double Value) {
        Core::FStats::Get().Record(*this, Value, false);
    }

    void Core::FStat::Set(double Value) {
        Core::FStats::Get().Record(*this, Value, true);
    }

    Core::FStats& Core::FStats::Get() {
        static Core::FStats Instance{};
        return Instance;
    }

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

    Core::Uint64 Core::FStats::GetFrameNumber() const {
        const std::lock_guard Lock{mMutex};
        return mFrameNumber;
    }

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

    void Core::FStats::Unregister(Core::FStat* Stat) {
        const std::lock_guard Lock{mMutex};

        std::erase(mStats, Stat);
        std::erase_if(mSnapshots, [Stat](const Core::FStatSnapshot& Snapshot) {
            return Snapshot.GetGroup() == Stat->mGroup && Snapshot.GetName() == Stat->mName;
        });
    }

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

            for (std::size_t Index{}; Index < Stat.mHistoryCount; ++Index) {
                const double Value{Stat.mHistory[Index]};
                Snapshot.mAverage += Value / static_cast<double>(Stat.mHistoryCount);
                Snapshot.mMinimum = std::min(Snapshot.mMinimum, Value);
                Snapshot.mMaximum = std::max(Snapshot.mMaximum, Value);
            }
        }

        return Snapshot;
    }

    Core::FStopwatch::FStopwatch()
		: mStart{std::chrono::steady_clock::now()} {
    }

    void Core::FStopwatch::Reset() {
        mStart = std::chrono::steady_clock::now();
    }

    double Core::FStopwatch::GetElapsedMilliseconds() const {
        return std::chrono::duration<double, std::milli>{std::chrono::steady_clock::now() - mStart}.count();
    }

    Core::FScopedStatTimer::FScopedStatTimer(Core::FStat& Stat)
		: mStat{&Stat} {
    }

    Core::FScopedStatTimer::~FScopedStatTimer() {
        mStat->Add(mWatch.GetElapsedMilliseconds());
    }

    Core::FScopedStatFrame::FScopedStatFrame()
		: mOwner{Core::FStats::Get().BeginFrame()} {
    }

    Core::FScopedStatFrame::~FScopedStatFrame() {
        if (mOwner) {
            Core::FStats::Get().EndFrame();
        }
    }

}
