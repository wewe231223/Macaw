#pragma once

#include "Script/LuaVM.h"
#include "Script/CoroutineScheduler.h"

class FLuaCoroutineRuntime final {
public:
    ReflectTypes(FLuaCoroutineRuntime);

public:
    FLuaCoroutineRuntime() = default;
    ~FLuaCoroutineRuntime();
    FLuaCoroutineRuntime(const FLuaCoroutineRuntime&) = delete;
    FLuaCoroutineRuntime& operator=(const FLuaCoroutineRuntime&) = delete;
    FLuaCoroutineRuntime(FLuaCoroutineRuntime&&) = delete;
    FLuaCoroutineRuntime& operator=(FLuaCoroutineRuntime&&) = delete;

public:
    ReflectStruct(FLuaCoroutineRuntime);
    bool Initialize();
    void Shutdown();
    void Update(float DeltaTime);
    FLuaVM& GetLuaVM();
    const FLuaVM& GetLuaVM() const;
    CoroutineScheduler& GetScheduler();
    const CoroutineScheduler& GetScheduler() const;

private:
    ReflectMembers();

private:
    FLuaVM mLuaVM{};
    ReflectProperty(mLuaVM, "LuaVM", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    CoroutineScheduler mScheduler{};
    ReflectProperty(mScheduler, "Scheduler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
