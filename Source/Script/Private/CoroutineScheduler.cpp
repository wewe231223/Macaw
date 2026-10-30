/**
 * @file CoroutineScheduler.cpp
 * @brief 시간·조건 대기 콜백과 Lua 코루틴의 실행 예약에 대한 구현입니다.
 */

#include "Script/CoroutineScheduler.h"
#include "Core/Stats.h"

#include <utility>

/**
 * @brief 스크립트 실행 비용과 대기 상태를 측정하는 내부 통계입니다.
 */
namespace {
    /**
     * @brief Lua 코루틴 스케줄러 갱신 시간를 측정하는 통계 항목입니다.
     */
    FStat SchedulerTime{"script", "Coroutine update CPU", EStatUnit::Milliseconds};
    /**
     * @brief 대기 중인 Lua 코루틴 수를 측정하는 통계 항목입니다.
     */
    FStat PendingWaits{"script", "Pending waits (sum of updates)"};
    /**
     * @brief Lua 코루틴 재개 처리 시간를 측정하는 통계 항목입니다.
     */
    FStat ResumeTime{"script", "Coroutine resume CPU", EStatUnit::Milliseconds};
}

CoroutineScheduler::CoroutineInfo::CoroutineInfo(CoroutineFunc Function, float WaitTime, CheckFunc Condition)
	: mRemainTime{WaitTime},
	  mCoroutineFunction{std::move(Function)},
	  mCheckFunction{std::move(Condition)} {
}

void CoroutineScheduler::Wait(CoroutineFunc Function, float WaitTime) { // Function은 Coroutine 대기 완료 후 재개할 함수 람다 형태로 추가
    CoroutineInfo Coroutine{std::move(Function), WaitTime, {}};
    mCoroutineInfoArray.push_back(std::move(Coroutine)); // CouroutineInfoArray에 해당 정보 추가
}

void CoroutineScheduler::Update(float DeltaTime) {
    const FScopedStatTimer StatTimer{SchedulerTime};
    PendingWaits.Add(static_cast<double>(mCoroutineInfoArray.size()));
    for (std::size_t Remaining{mCoroutineInfoArray.size()}; Remaining > 0; --Remaining) {
        const std::size_t Index{Remaining - 1};
        if (mCoroutineInfoArray[Index].mCheckFunction) { // 조건 Coroutine일 경우와 시간 Coroutine일 경우 분리
            if (mCoroutineInfoArray[Index].mCheckFunction()) {
                CoroutineFunc Callback{mCoroutineInfoArray[Index].mCoroutineFunction}; // Callback으로 실행
                Callback();
                mCoroutineInfoArray.erase(mCoroutineInfoArray.begin() + static_cast<std::ptrdiff_t>(Index)); // Callback 실행했으므로 Coroutine 목록에서 제거
            }
        } else {
            mCoroutineInfoArray[Index].mRemainTime -= DeltaTime; // 시간 DeltaTime만큼 감소
            if (mCoroutineInfoArray[Index].mRemainTime <= 0.0f) {
                CoroutineFunc Callback{mCoroutineInfoArray[Index].mCoroutineFunction}; // Callback으로 실행
                Callback();
                mCoroutineInfoArray.erase(mCoroutineInfoArray.begin() + static_cast<std::ptrdiff_t>(Index)); // Callback 실행했으므로 Coroutine 목록에서 제거
            }
        }
    }
}

void CoroutineScheduler::WaitUntil(CoroutineFunc Function, CheckFunc Condition) {
    CoroutineInfo Coroutine{std::move(Function), 0.0f, std::move(Condition)};
    mCoroutineInfoArray.push_back(std::move(Coroutine)); // CouroutineInfoArray에 해당 정보 추가
}

void CoroutineScheduler::RegisterLuaCoroutine(sol::thread LuaCoroutine) {
    mLuaCoroutineArray.push_back(std::move(LuaCoroutine));
}

void CoroutineScheduler::RunCoroutine(sol::state& Lua, sol::protected_function Resume, sol::thread LuaCoroutine) { // Coroutine 반복 실행함수, Yield 만나고 값에 따라서 Wait Array에 값을 넣는 역할 담당
    const FScopedStatTimer StatTimer{ResumeTime};
    auto YieldResult{Resume(LuaCoroutine)}; // Yield시 같이 넘기는 값을 Resume하면서 가져옴
    if (!YieldResult.valid()) {
        return;
    }

    const bool Success{YieldResult.get<bool>(0)};
    if (Success) { // Yield가 Success일 경우만 실행
        sol::protected_function Status{Lua["coroutine"]["status"]}; // Coroutine.status 실행해서 현재 상태 체크
        const auto StatusResult{Status(LuaCoroutine)};
        if (!StatusResult.valid()) {
            return;
        }

        const FString State{StatusResult.get<FString>()};

        if (State == "dead") { // 함수가 끝났으므로 대기 정보를 읽지 않음
            return;
        }

        if (State == "suspended") { // 대기 상태이므로 Wait 체크
            const FString WaitType{YieldResult.get<FString>(1)};
            if (WaitType == "wait") {
                const float WaitTime{YieldResult.get<float>(2)};
                Wait([this, Resume, LuaCoroutine, &Lua]() { // Wait가 있으면 대기열에 현재 람다를 넣음, 현재 람다가 끝나고 다음 Wait에도 실행되도록 RunCoroutine을 넣음
                    RunCoroutine(Lua, Resume, LuaCoroutine);
                }, WaitTime);
            }
            if (WaitType == "wait_until") {
                const sol::protected_function Condition{YieldResult.get<sol::protected_function>(2)};
                CheckFunc CheckFunction{[Condition]() {
                    const auto Result{Condition()};
                    return Result.valid() && Result.get<bool>();
                }};
                WaitUntil([this, Resume, LuaCoroutine, &Lua]() { // WaitUntil이 있으면 대기열에 현재 람다를 넣음, 현재 람다가 끝나고 다음 Wait에도 실행되도록 RunCoroutine을 넣음
                    RunCoroutine(Lua, Resume, LuaCoroutine);
                }, std::move(CheckFunction));
            }
        }
    }
}

void CoroutineScheduler::Clear() {
    mCoroutineInfoArray.clear();
    mLuaCoroutineArray.clear();
}

const TArray<sol::thread>& CoroutineScheduler::GetLuaCoroutines() const {
    return mLuaCoroutineArray;
}
