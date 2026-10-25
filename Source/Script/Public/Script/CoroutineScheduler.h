/**
 * @file CoroutineScheduler.h
 * @brief 시간·조건 대기 콜백과 Lua 코루틴의 실행 예약에 대한 선언입니다.
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <functional>
#include <sol/sol.hpp>

/**
 * @brief 시간·조건 대기 콜백과 Lua 코루틴의 실행 예약을 담당합니다.
 */
class CoroutineScheduler final {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(CoroutineScheduler);
    /**
     * @brief 대기 완료 후 실행할 인수 없는 콜백 타입입니다.
     */
    using CoroutineFunc = std::function<void()>;
    /**
     * @brief 대기 완료 조건을 반환하는 검사 함수 타입입니다.
     */
    using CheckFunc = std::function<bool()>;

public:
    /**
     * @brief 대기 시간·조건과 완료 콜백을 함께 보관하는 예약 항목입니다.
     */
    struct CoroutineInfo {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(CoroutineInfo);

    public:
        /**
         * @brief 전달받은 설정과 의존성으로 CoroutineInfo의 초기 상태를 구성합니다.
         * @param[in] Function 대기가 끝났을 때 한 번 실행할 콜백입니다.
         * @param[in] WaitTime 콜백 실행까지 기다릴 시간이며 초 단위입니다.
         * @param[in] Condition 대기를 완료할 때 true를 반환하는 조건 검사 콜백입니다.
         */
        CoroutineInfo(CoroutineFunc Function, float WaitTime, CheckFunc Condition);

    public:
        /**
         * @brief 이 타입의 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(CoroutineInfo);

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();
        /**
         * @brief 스케줄러가 예약 항목의 대기 상태와 콜백에 접근하도록 허용합니다.
         */
        friend class CoroutineScheduler;

    private:
        /**
         * @brief 시간 대기가 완료될 때까지 남은 초 단위 시간입니다.
         */
        float mRemainTime{}; // 남은 시간
        ReflectProperty(mRemainTime, "RemainTime", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        /**
         * @brief 시간 또는 조건 대기가 끝났을 때 실행할 콜백입니다.
         */
        CoroutineFunc mCoroutineFunction{}; // Couroutine 대기 완료 후 재개할 함수
        ReflectProperty(mCoroutineFunction, "CoroutineFunction", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        /**
         * @brief 대기 완료 조건을 검사하는 선택적 콜백입니다.
         */
        CheckFunc mCheckFunction{}; // 조건 Coroutine사 체크할 함수
        ReflectProperty(mCheckFunction, "CheckFunction", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    /**
     * @brief CoroutineScheduler의 초기 상태를 구성합니다.
     */
    CoroutineScheduler() = default;
    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    ~CoroutineScheduler() = default;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    CoroutineScheduler(const CoroutineScheduler&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    CoroutineScheduler& operator=(const CoroutineScheduler&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    CoroutineScheduler(CoroutineScheduler&&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    CoroutineScheduler& operator=(CoroutineScheduler&&) = delete;

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(CoroutineScheduler);
    /**
     * @brief 지정한 시간이 지난 뒤 콜백을 한 번 실행하도록 예약합니다.
     * @param[in] Function 대기가 끝났을 때 한 번 실행할 콜백입니다.
     * @param[in] WaitTime 콜백 실행까지 기다릴 시간이며 초 단위입니다.
     */
    void Wait(CoroutineFunc Function, float WaitTime);
    /**
     * @brief 대기 시간을 줄이고 조건 또는 시간이 충족된 콜백을 실행하여 제거합니다.
     * @param[in] DeltaTime 이전 갱신 이후의 경과 시간이며 초 단위입니다.
     */
    void Update(float DeltaTime);
    /**
     * @brief 조건 함수가 true를 반환하면 콜백을 한 번 실행하도록 예약합니다.
     * @param[in] Function 대기가 끝났을 때 한 번 실행할 콜백입니다.
     * @param[in] Condition 대기를 완료할 때 true를 반환하는 조건 검사 콜백입니다.
     */
    void WaitUntil(CoroutineFunc Function, CheckFunc Condition);
    /**
     * @brief Lua 코루틴을 보관하여 예약된 실행 동안 수명을 유지합니다.
     * @param[in] LuaCoroutine 등록하거나 재개할 Lua 코루틴 스레드입니다.
     */
    void RegisterLuaCoroutine(sol::thread LuaCoroutine);
    /**
     * @brief Lua 코루틴을 재개하고 yield한 시간 또는 조건에 따라 다음 실행을 예약합니다.
     * @param[in,out] Lua 코루틴을 실행할 Lua 상태입니다.
     * @param[in] Resume 코루틴 재개에 사용하는 Lua 함수입니다.
     * @param[in] LuaCoroutine 등록하거나 재개할 Lua 코루틴 스레드입니다.
     */
    void RunCoroutine(sol::state& Lua, sol::protected_function Resume, sol::thread LuaCoroutine);
    /**
     * @brief 예약된 콜백과 보관 중인 Lua 코루틴을 모두 해제합니다.
     */
    void Clear();
    /**
     * @brief 스케줄러가 수명을 유지하는 Lua 코루틴 목록을 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const TArray<sol::thread>& GetLuaCoroutines() const;

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief Lua 코루틴의 수명을 유지하는 스레드 목록입니다.
     */
    TArray<sol::thread> mLuaCoroutineArray{};
    ReflectProperty(mLuaCoroutineArray, "LuaCoroutineArray", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 시간 또는 조건 충족을 기다리는 콜백 예약 목록입니다.
     */
    TArray<CoroutineInfo> mCoroutineInfoArray{};
    ReflectProperty(mCoroutineInfoArray, "CoroutineInfoArray", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
