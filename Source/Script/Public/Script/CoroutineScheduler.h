#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <functional>
#include <sol/sol.hpp>

class CoroutineScheduler final {
public:
    ReflectTypes(CoroutineScheduler);
    using CoroutineFunc = std::function<void()>;
    using CheckFunc = std::function<bool()>;

public:
    struct CoroutineInfo {
    public:
        ReflectTypes(CoroutineInfo);

    public:
        CoroutineInfo(CoroutineFunc Function, float WaitTime, CheckFunc Condition);

    public:
        ReflectStruct(CoroutineInfo);

    private:
        ReflectMembers();
        friend class CoroutineScheduler;

    private:
        float mRemainTime{}; // 남은 시간
        ReflectProperty(mRemainTime, "RemainTime", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        CoroutineFunc mCoroutineFunction{}; // Couroutine 대기 완료 후 재개할 함수
        ReflectProperty(mCoroutineFunction, "CoroutineFunction", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        CheckFunc mCheckFunction{}; // 조건 Coroutine사 체크할 함수
        ReflectProperty(mCheckFunction, "CheckFunction", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    CoroutineScheduler() = default;
    ~CoroutineScheduler() = default;
    CoroutineScheduler(const CoroutineScheduler&) = delete;
    CoroutineScheduler& operator=(const CoroutineScheduler&) = delete;
    CoroutineScheduler(CoroutineScheduler&&) = delete;
    CoroutineScheduler& operator=(CoroutineScheduler&&) = delete;

public:
    ReflectStruct(CoroutineScheduler);
    void Wait(CoroutineFunc Function, float WaitTime);
    void Update(float DeltaTime);
    void WaitUntil(CoroutineFunc Function, CheckFunc Condition);
    void RegisterLuaCoroutine(sol::thread LuaCoroutine);
    void RunCoroutine(sol::state& Lua, sol::protected_function Resume, sol::thread LuaCoroutine);
    void Clear();
    const TArray<sol::thread>& GetLuaCoroutines() const;

private:
    ReflectMembers();

private:
    TArray<sol::thread> mLuaCoroutineArray{};
    ReflectProperty(mLuaCoroutineArray, "LuaCoroutineArray", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<CoroutineInfo> mCoroutineInfoArray{};
    ReflectProperty(mCoroutineInfoArray, "CoroutineInfoArray", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
