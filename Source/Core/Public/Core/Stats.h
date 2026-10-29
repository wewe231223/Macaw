#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <chrono>
#include <mutex>

enum class EStatUnit : Uint8 {
    Count,
    Milliseconds,
    Bytes,
    Hertz,
    Percent
};

enum class EStatMode : Uint8 {
    Counter,
    Gauge
};

class FStatSnapshot {
public:
    ReflectTypes(FStatSnapshot);

public:
    FStatSnapshot() = default;

public:
    ReflectStruct(FStatSnapshot);
    const FString& GetGroup() const;
    const FString& GetName() const;
    EStatUnit GetUnit() const;
    double GetValue() const;
    double GetAverage() const;
    double GetMinimum() const;
    double GetMaximum() const;
    Uint64 GetSamples() const;
    std::size_t GetFrameCount() const;

private:
    friend class FStats;
    ReflectMembers();

private:
    FString mGroup{};
    ReflectProperty(mGroup, "Group", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    EStatUnit mUnit{};
    ReflectProperty(mUnit, "Unit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    double mValue{};
    ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    double mAverage{};
    ReflectProperty(mAverage, "Average", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    double mMinimum{};
    ReflectProperty(mMinimum, "Minimum", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    double mMaximum{};
    ReflectProperty(mMaximum, "Maximum", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mSamples{};
    ReflectProperty(mSamples, "Samples", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mFrameCount{};
    ReflectProperty(mFrameCount, "FrameCount", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FStat {
public:
    ReflectTypes(FStat);

public:
    FStat(FString Group, FString Name, EStatUnit Unit = EStatUnit::Count, EStatMode Mode = EStatMode::Counter);
    ~FStat();
    FStat(const FStat& Other) = delete;
    FStat& operator=(const FStat& Other) = delete;
    FStat(FStat&& Other) = delete;
    FStat& operator=(FStat&& Other) = delete;

public:
    ReflectStruct(FStat);
    void Add(double Value = 1.0);
    void Set(double Value);

private:
    friend class FStats;
    ReflectMembers();

private:
    FString mGroup{};
    ReflectProperty(mGroup, "Group", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    EStatUnit mUnit{};
    ReflectProperty(mUnit, "Unit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    EStatMode mMode{};
    ReflectProperty(mMode, "Mode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    double mValue{};
    ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mSamples{};
    ReflectProperty(mSamples, "Samples", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<double, 120> mHistory{};
    ReflectProperty(mHistory, "History", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mHistoryIndex{};
    ReflectProperty(mHistoryIndex, "HistoryIndex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mHistoryCount{};
    ReflectProperty(mHistoryCount, "HistoryCount", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FStats {
public:
    ReflectTypes(FStats);

private:
    FStats() = default;
    FStats(const FStats& Other) = delete;
    FStats& operator=(const FStats& Other) = delete;

public:
    ReflectStruct(FStats);
    static FStats& Get();
    bool BeginFrame();
    bool EndFrame();
    void Reset();
    Uint64 GetFrameNumber() const;
    TArray<FString> GetGroups() const;
    TArray<FStatSnapshot> GetSnapshots(const FString& Group = {}) const;

private:
    friend class FStat;
    ReflectMembers();
    void Register(FStat* Stat);
    void Unregister(FStat* Stat);
    void Record(FStat& Stat, double Value, bool Replace);
    FStatSnapshot MakeSnapshot(const FStat& Stat) const;

private:
    mutable std::mutex mMutex{};
    ReflectProperty(mMutex, "Mutex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FStat*> mStats{};
    ReflectProperty(mStats, "Stats", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FStatSnapshot> mSnapshots{};
    ReflectProperty(mSnapshots, "Snapshots", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::chrono::steady_clock::time_point mFrameStart{};
    ReflectProperty(mFrameStart, "FrameStart", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mFrameNumber{};
    ReflectProperty(mFrameNumber, "FrameNumber", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mInFrame{};
    ReflectProperty(mInFrame, "InFrame", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FStopwatch {
public:
    ReflectTypes(FStopwatch);

public:
    FStopwatch();

public:
    ReflectStruct(FStopwatch);
    void Reset();
    double GetElapsedMilliseconds() const;

private:
    ReflectMembers();

private:
    std::chrono::steady_clock::time_point mStart{};
    ReflectProperty(mStart, "Start", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FScopedStatTimer {
public:
    ReflectTypes(FScopedStatTimer);

public:
    explicit FScopedStatTimer(FStat& Stat);
    ~FScopedStatTimer();
    FScopedStatTimer(const FScopedStatTimer& Other) = delete;
    FScopedStatTimer& operator=(const FScopedStatTimer& Other) = delete;
    FScopedStatTimer(FScopedStatTimer&& Other) = delete;
    FScopedStatTimer& operator=(FScopedStatTimer&& Other) = delete;

public:
    ReflectStruct(FScopedStatTimer);

private:
    ReflectMembers();

private:
    FStat* mStat{};
    ReflectProperty(mStat, "Stat", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FStopwatch mWatch{};
    ReflectProperty(mWatch, "Watch", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FScopedStatFrame {
public:
    ReflectTypes(FScopedStatFrame);

public:
    FScopedStatFrame();
    ~FScopedStatFrame();
    FScopedStatFrame(const FScopedStatFrame& Other) = delete;
    FScopedStatFrame& operator=(const FScopedStatFrame& Other) = delete;
    FScopedStatFrame(FScopedStatFrame&& Other) = delete;
    FScopedStatFrame& operator=(FScopedStatFrame&& Other) = delete;

public:
    ReflectStruct(FScopedStatFrame);

private:
    ReflectMembers();

private:
    bool mOwner{};
    ReflectProperty(mOwner, "Owner", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
