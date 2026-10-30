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

/**
 * @brief 프레임·메모리·객체 수 통계와 프레임 간격 측정 상태입니다.
 */
namespace {
    /**
     * @brief 프레젠테이션을 포함한 프레임 작업 시간를 측정하는 통계 항목입니다.
     */
    FStat FrameTime{"unit", "Frame work incl. present", EStatUnit::Milliseconds, EStatMode::Gauge};

    /**
     * @brief 연속 프레임 시작 사이의 밀리초 간격를 측정하는 통계 항목입니다.
     */
    FStat FrameInterval{"fps", "Frame interval", EStatUnit::Milliseconds, EStatMode::Gauge};

    /**
     * @brief 프레임 시작 간격으로 계산한 초당 프레임 수를 측정하는 통계 항목입니다.
     */
    FStat FrameRate{"fps", "FPS", EStatUnit::Hertz, EStatMode::Gauge};

    /**
     * @brief 현재 살아 있는 UObject 수를 측정하는 통계 항목입니다.
     */
    FStat LiveObjects{"scene", "Live UObjects (global)", EStatUnit::Count, EStatMode::Gauge};

    /**
     * @brief 프로세스에서 생성한 UObject 누적 수를 측정하는 통계 항목입니다.
     */
    FStat CreatedObjects{"scene", "Created UObjects (global)", EStatUnit::Count, EStatMode::Gauge};

    /**
     * @brief 추적 중인 UObject 힙 바이트 수를 측정하는 통계 항목입니다.
     */
    FStat HeapBytes{"memory", "UObject heap (tracked)", EStatUnit::Bytes, EStatMode::Gauge};

    /**
     * @brief UObject 힙 사용량의 최대 바이트 수를 측정하는 통계 항목입니다.
     */
    FStat PeakHeapBytes{"memory", "Peak UObject heap", EStatUnit::Bytes, EStatMode::Gauge};

    /**
     * @brief 연속 프레임의 시작 간격을 계산하기 위한 이전 시작 시각입니다.
     */
    std::chrono::steady_clock::time_point PreviousFrameStart{};
}

const FString& FStatSnapshot::GetGroup() const {
    return mGroup;
}

const FString& FStatSnapshot::GetName() const {
    return mName;
}

EStatUnit FStatSnapshot::GetUnit() const {
    return mUnit;
}

double FStatSnapshot::GetValue() const {
    return mValue;
}

double FStatSnapshot::GetAverage() const {
    return mAverage;
}

double FStatSnapshot::GetMinimum() const {
    return mMinimum;
}

double FStatSnapshot::GetMaximum() const {
    return mMaximum;
}

Uint64 FStatSnapshot::GetSamples() const {
    return mSamples;
}

std::size_t FStatSnapshot::GetFrameCount() const {
    return mFrameCount;
}

FStat::FStat(FString Group, FString Name, EStatUnit Unit, EStatMode Mode)
	: mGroup{std::move(Group)},
	  mName{std::move(Name)},
	  mUnit{Unit},
	  mMode{Mode} {
    std::transform(mGroup.begin(), mGroup.end(), mGroup.begin(), [](unsigned char Character) {
        return static_cast<char>(std::tolower(Character));
    });

    FStats::Get().Register(this);
}

FStat::~FStat() {
    FStats::Get().Unregister(this);
}

void FStat::Add(double Value) {
    FStats::Get().Record(*this, Value, false);
}

void FStat::Set(double Value) {
    FStats::Get().Record(*this, Value, true);
}

FStats& FStats::Get() {
    static FStats Instance{};
    return Instance;
}

bool FStats::BeginFrame() {
    const std::lock_guard Lock{mMutex};

    if (mInFrame) {
        return false;
    }

    mFrameStart = std::chrono::steady_clock::now();

    const double Interval{PreviousFrameStart != std::chrono::steady_clock::time_point{} ? std::chrono::duration<double, std::milli>{mFrameStart - PreviousFrameStart}.count() : 0.0};

    PreviousFrameStart = mFrameStart;

    for (FStat* Stat : mStats) {
        if (Stat->mMode == EStatMode::Counter) {
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

bool FStats::EndFrame() {
    const std::lock_guard Lock{mMutex};

    if (!mInFrame) {
        return false;
    }

    FrameTime.mValue = std::chrono::duration<double, std::milli>{std::chrono::steady_clock::now() - mFrameStart}.count();
    FrameTime.mSamples = 1;

    const FMemoryStats Memory{FMemory::GetStats()};
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

    for (FStat* Stat : mStats) {
        Stat->mHistory[Stat->mHistoryIndex] = Stat->mValue;
        Stat->mHistoryIndex = (Stat->mHistoryIndex + 1) % Stat->mHistory.size();
        Stat->mHistoryCount = std::min(Stat->mHistoryCount + 1, Stat->mHistory.size());
        mSnapshots.push_back(MakeSnapshot(*Stat));
    }

    std::sort(mSnapshots.begin(), mSnapshots.end(), [](const FStatSnapshot& First, const FStatSnapshot& Second) {
        return First.GetGroup() != Second.GetGroup() ? First.GetGroup() < Second.GetGroup() : First.GetName() < Second.GetName();
    });

    ++mFrameNumber;
    mInFrame = false;

    return true;
}

void FStats::Reset() {
    const std::lock_guard Lock{mMutex};

    for (FStat* Stat : mStats) {
        if (Stat->mMode == EStatMode::Counter) {
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

Uint64 FStats::GetFrameNumber() const {
    const std::lock_guard Lock{mMutex};
    return mFrameNumber;
}

TArray<FString> FStats::GetGroups() const {
    const std::lock_guard Lock{mMutex};
    TArray<FString> Groups{};

    for (const FStat* Stat : mStats) {
        if (std::find(Groups.begin(), Groups.end(), Stat->mGroup) == Groups.end()) {
            Groups.push_back(Stat->mGroup);
        }
    }

    std::sort(Groups.begin(), Groups.end());

    return Groups;
}

TArray<FStatSnapshot> FStats::GetSnapshots(const FString& Group) const {
    const std::lock_guard Lock{mMutex};
    TArray<FStatSnapshot> Result{};

    for (const FStatSnapshot& Snapshot : mSnapshots) {
        if (Group.empty() || Snapshot.GetGroup() == Group) {
            Result.push_back(Snapshot);
        }
    }

    return Result;
}

void FStats::Register(FStat* Stat) {
    const std::lock_guard Lock{mMutex};

    if (Stat->mGroup.empty() || Stat->mName.empty() || Stat->mGroup == "all" || Stat->mGroup == "none" || Stat->mGroup == "list" || Stat->mGroup == "reset" || Stat->mGroup == "shadows" || std::any_of(Stat->mGroup.begin(), Stat->mGroup.end(), [](unsigned char Character) {
        return Character == 0 || std::isspace(Character) != 0;
    }) || std::any_of(mStats.begin(), mStats.end(), [Stat](const FStat* Existing) {
        return Existing->mGroup == Stat->mGroup && Existing->mName == Stat->mName;
    })) {
        throw std::invalid_argument{"A stat requires a nonempty, nonreserved group token and a unique name within the group"};
    }

    mStats.push_back(Stat);
}

void FStats::Unregister(FStat* Stat) {
    const std::lock_guard Lock{mMutex};

    std::erase(mStats, Stat);
    std::erase_if(mSnapshots, [Stat](const FStatSnapshot& Snapshot) {
        return Snapshot.GetGroup() == Stat->mGroup && Snapshot.GetName() == Stat->mName;
    });
}

void FStats::Record(FStat& Stat, double Value, bool Replace) {
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

FStatSnapshot FStats::MakeSnapshot(const FStat& Stat) const {
    FStatSnapshot Snapshot{};
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

FStopwatch::FStopwatch()
	: mStart{std::chrono::steady_clock::now()} {
}

void FStopwatch::Reset() {
    mStart = std::chrono::steady_clock::now();
}

double FStopwatch::GetElapsedMilliseconds() const {
    return std::chrono::duration<double, std::milli>{std::chrono::steady_clock::now() - mStart}.count();
}

FScopedStatTimer::FScopedStatTimer(FStat& Stat)
	: mStat{&Stat} {
}

FScopedStatTimer::~FScopedStatTimer() {
    mStat->Add(mWatch.GetElapsedMilliseconds());
}

FScopedStatFrame::FScopedStatFrame()
	: mOwner{FStats::Get().BeginFrame()} {
}

FScopedStatFrame::~FScopedStatFrame() {
    if (mOwner) {
        FStats::Get().EndFrame();
    }
}
