/**
 * @file FScopedFlag.h
 * @brief 스코프 동안 불리언 플래그를 임시로 설정하고 소멸 시 원래 값을 복원하는 타입을 정의한다.
 */

#pragma once

/**
 * @brief 불리언 플래그의 이전 값을 보관하고 객체 소멸 시 복원한다.
 * @details 생성 시 플래그의 현재 값을 저장한 뒤 지정한 값을 설정한다. 동일한 플래그를 중첩 스코프에서 사용하면 각 객체가 자신의 생성 직전 값으로 복원한다.
 * @note 관리 대상 플래그는 이 객체가 소멸할 때까지 유효해야 한다.
 */
class FScopedFlag final {
public:
    /**
     * @brief 플래그의 이전 값을 저장하고 지정한 임시 값을 설정한다.
     * @param[in,out] Flag 값을 설정하고 소멸 시 복원할 플래그.
     * @param Value 생성 시 플래그에 설정할 값. 기본값은 true.
     */
    explicit FScopedFlag(bool& Flag, bool Value = true);

    /**
     * @brief 관리 대상 플래그를 생성 직전에 저장한 값으로 복원한다.
     * @details 스코프 안에서 플래그가 다른 값으로 변경되었더라도 저장한 이전 값을 복원한다.
     */
    ~FScopedFlag();

    /** @brief 플래그 복원 책임을 가진 객체의 복사 생성을 금지한다. */
    FScopedFlag(const FScopedFlag&) = delete;

    /** @brief 플래그 복원 책임을 가진 객체의 복사 할당을 금지한다. */
    FScopedFlag& operator=(const FScopedFlag&) = delete;

    /** @brief 플래그 복원 책임을 가진 객체의 이동 생성을 금지한다. */
    FScopedFlag(FScopedFlag&&) = delete;

    /** @brief 플래그 복원 책임을 가진 객체의 이동 할당을 금지한다. */
    FScopedFlag& operator=(FScopedFlag&&) = delete;

private:
    /** @brief 외부에서 소유하는 관리 대상 플래그의 참조. */
    bool& mFlag;

    /** @brief 생성 시 저장한 플래그 값. 소멸 시 이 값으로 복원한다. */
    bool mPreviousValue{};
};
