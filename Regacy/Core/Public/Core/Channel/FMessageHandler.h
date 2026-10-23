/**
 * @file FMessageHandler.h
 * @brief 메시지 타입 하나와 호출 대상을 연결하는 이동 전용 핸들러를 정의한다.
 */
#pragma once

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include "Core/Base/ErrorHandler.h"
#include "Core/Channel/FMessage.h"

/**
 * @brief 메시지의 정적 타입 정보와 구체적인 호출 대상을 보관한다.
 * @details Create()로 등록한 메시지 타입을 const 참조로 호출 대상에 전달하며 호출 대상의 반환값은 사용하지 않는다.
 */
class FMessageHandler {
public:
    /**
     * @brief 타입 정보와 호출 대상이 없는 핸들러를 생성한다.
     */
    FMessageHandler() = default;
    /**
     * @brief 저장된 호출 대상과 그 소유 자원을 정리한다.
     */
    ~FMessageHandler() noexcept = default;

    /**
     * @brief 핸들러의 복사 생성을 금지한다.
     */
    FMessageHandler(const FMessageHandler&) = delete;
    /**
     * @brief 핸들러의 복사 할당을 금지한다.
     */
    FMessageHandler& operator=(const FMessageHandler&) = delete;

    /**
     * @brief 타입 정보와 호출 대상을 이동하여 핸들러를 생성한다.
     */
    FMessageHandler(FMessageHandler&&) = default;
    /**
     * @brief 타입 정보와 호출 대상을 이동하여 현재 핸들러를 갱신한다.
     */
    FMessageHandler& operator=(FMessageHandler&&) = default;

public:
    /**
     * @brief 지정한 메시지 타입에 대한 호출 대상을 저장한 핸들러를 생성한다.
     * @tparam TMessage 처리할 메시지 타입.
     * @tparam TCallable 복사 생성 가능하고 const TMessage 참조로 호출 가능한 타입.
     * @param Callable 저장할 호출 대상. 값으로 보관하며 전달한 인수의 값 범주에 따라 복사하거나 이동한다.
     * @return 메시지 타입과 호출 대상이 등록된 핸들러.
     */
    template <CMessageType TMessage, typename TCallable>
        requires std::copy_constructible<std::decay_t<TCallable>> && std::invocable<std::decay_t<TCallable>&, const TMessage&>
    static FMessageHandler Create(TCallable&& Callable);

public:
    /**
     * @brief 지정한 타입 정보를 이 핸들러가 처리하는지 확인한다.
     * @param Type 확인할 타입 정보 포인터. nullptr이면 false이다.
     * @return 등록된 타입 정보와 주소가 같으면 true, 그 외에는 false.
     * @pre 핸들러에 메시지 타입 정보가 등록되어 있어야 한다.
     */
    [[nodiscard]] bool Handles(const FMessageTypeInfo* Type) const noexcept;
    /**
     * @brief 등록된 메시지 타입 정보를 조회한다.
     * @return 등록된 정적 타입 정보의 읽기 전용 참조.
     * @details 등록된 타입 정보가 없으면 치명적 오류를 보고하고 프로세스를 종료한다.
     */
    [[nodiscard]] const FMessageTypeInfo& GetMessageType() const noexcept;

    /**
     * @brief 저장된 호출 대상에 메시지 페이로드를 전달한다.
     * @param Message 등록된 타입과 일치하는 유효한 메시지.
     * @details 호출 대상이 없거나 메시지 타입이 일치하지 않으면 치명적 오류를 보고한다. 호출 대상이 던진 예외는 호출자에게 전달된다.
     */
    void Invoke(const FMessage& Message);

private:
    /**
     * @brief 처리할 메시지 타입의 외부 정적 타입 정보 포인터.
     */
    const FMessageTypeInfo* mMessageType{nullptr};
    /**
     * @brief 구체적인 메시지 타입 검사와 호출 대상 실행을 수행하는 함수 객체.
     */
    std::function<void(const FMessage&)> mFunction{};
};

template <CMessageType TMessage, typename TCallable>
    requires std::copy_constructible<std::decay_t<TCallable>> && std::invocable<std::decay_t<TCallable>&, const TMessage&>
FMessageHandler FMessageHandler::Create(TCallable&& Callable) {
    using FCallable = std::decay_t<TCallable>;

    FMessageHandler Result{};

    Result.mMessageType = &TMessage::StaticTypeInfo();
    Result.mFunction = [Callable = FCallable(std::forward<TCallable>(Callable))](const FMessage& Message) mutable {
        const TMessage* TypedMessage{Message.Get<TMessage>()};

        if (TypedMessage == nullptr) {
            ErrorHandler::Report("FMessageHandler::Create", "The message type does not match the handler's registered type.", ErrorHandler::EErrorLevel::Critical);
        }

        std::invoke(Callable, *TypedMessage);
    };

    return Result;
}
