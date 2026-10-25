/**
 * @file LuaCoroutineRuntime.h
 * @brief Lua VM과 코루틴 스케줄러의 초기화 및 프레임 갱신에 대한 선언입니다.
 */

#pragma once

#include "Script/LuaVM.h"
#include "Script/CoroutineScheduler.h"

/**
 * @brief Lua VM과 코루틴 스케줄러의 초기화 및 프레임 갱신를 담당합니다.
 */
class FLuaCoroutineRuntime final {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FLuaCoroutineRuntime);

public:
    /**
     * @brief FLuaCoroutineRuntime의 초기 상태를 구성합니다.
     */
    FLuaCoroutineRuntime() = default;
    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    ~FLuaCoroutineRuntime();
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FLuaCoroutineRuntime(const FLuaCoroutineRuntime&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FLuaCoroutineRuntime& operator=(const FLuaCoroutineRuntime&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FLuaCoroutineRuntime(FLuaCoroutineRuntime&&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FLuaCoroutineRuntime& operator=(FLuaCoroutineRuntime&&) = delete;

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FLuaCoroutineRuntime);
    /**
     * @brief Lua VM을 초기화하고 시간·조건 대기를 제공하는 코루틴 API를 등록합니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool Initialize();
    /**
     * @brief 예약된 코루틴을 정리한 뒤 Lua VM을 종료합니다.
     */
    void Shutdown();
    /**
     * @brief 코루틴 스케줄러에 프레임 경과 시간을 전달합니다.
     * @param[in] DeltaTime 이전 갱신 이후의 경과 시간이며 초 단위입니다.
     */
    void Update(float DeltaTime);
    /**
     * @brief 런타임이 소유한 Lua VM의 참조를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    FLuaVM& GetLuaVM();
    /**
     * @brief 런타임이 소유한 Lua VM의 참조를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const FLuaVM& GetLuaVM() const;
    /**
     * @brief 런타임이 소유한 코루틴 스케줄러의 참조를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    CoroutineScheduler& GetScheduler();
    /**
     * @brief 런타임이 소유한 코루틴 스케줄러의 참조를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const CoroutineScheduler& GetScheduler() const;

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 스크립트 실행과 API 바인딩을 제공하는 소유 Lua VM입니다.
     */
    FLuaVM mLuaVM{};
    ReflectProperty(mLuaVM, "LuaVM", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief Lua 코루틴의 시간·조건 대기를 관리하는 스케줄러입니다.
     */
    CoroutineScheduler mScheduler{};
    ReflectProperty(mScheduler, "Scheduler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
