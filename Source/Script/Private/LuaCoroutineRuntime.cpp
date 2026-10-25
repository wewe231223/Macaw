/**
 * @file LuaCoroutineRuntime.cpp
 * @brief Lua VM과 코루틴 스케줄러의 초기화 및 프레임 갱신에 대한 구현입니다.
 */

#include "Script/LuaCoroutineRuntime.h"

FLuaCoroutineRuntime::~FLuaCoroutineRuntime() {
    Shutdown();
}

bool FLuaCoroutineRuntime::Initialize() {
    if (mLuaVM.IsInitialized()) {
        return true;
    }

    if (!mLuaVM.Initialize()) {
        return false;
    }

    sol::state& Lua{*mLuaVM.GetState()};

    // Lua Coroutine용 함수 Binding
    Lua.open_libraries(sol::lib::base, sol::lib::coroutine);
    Lua.set_function("RegisterCoroutine", [this](sol::thread LuaCoroutine) { // Coroutine에 대한 정보를 Scheduler에서 저장 (Coroutine이 다수일경우 구분해서 처리 필요)
        mScheduler.RegisterLuaCoroutine(LuaCoroutine);
    });
    Lua.set_function("StartCoroutine", [this](sol::thread LuaCoroutine) { // Coroutine 시작 후 Yield결과를 판정, Wait 및 WaitUntil 처리
        sol::state& State{*mLuaVM.GetState()};
        sol::protected_function Resume{State["coroutine"]["resume"]};
        mScheduler.RunCoroutine(State, Resume, LuaCoroutine);
    });

    if (!mLuaVM.ExecuteString("function Wait(Seconds) coroutine.yield(\"wait\", Seconds) end\nfunction WaitUntil(Condition) coroutine.yield(\"wait_until\", Condition) end", "Coroutine")) {
        Shutdown();
        return false;
    }

    return true;
}

void FLuaCoroutineRuntime::Shutdown() {
    mScheduler.Clear();
    mLuaVM.Shutdown();
}

void FLuaCoroutineRuntime::Update(float DeltaTime) {
    if (mLuaVM.IsInitialized()) {
        mScheduler.Update(DeltaTime);
    }
}

FLuaVM& FLuaCoroutineRuntime::GetLuaVM() {
    return mLuaVM;
}

const FLuaVM& FLuaCoroutineRuntime::GetLuaVM() const {
    return mLuaVM;
}

CoroutineScheduler& FLuaCoroutineRuntime::GetScheduler() {
    return mScheduler;
}

const CoroutineScheduler& FLuaCoroutineRuntime::GetScheduler() const {
    return mScheduler;
}
