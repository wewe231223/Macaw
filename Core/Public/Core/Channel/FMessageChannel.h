/**
 * @file FMessageChannel.h
 * @brief 메시지 큐, 타입별 핸들러 등록, 송신 인터페이스 및 디스패치 결과를 정의한다.
 */
#pragma once

#include <cstddef>
#include <deque>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include "Core/Channel/FMessage.h"
#include "Core/Channel/FMessageHandler.h"

/**
 * @brief 한 번의 Dispatch() 호출에서 처리한 메시지 수와 재진입 지연 여부를 보관한다.
 */
struct FMessageDispatchResult {
    /**
     * @brief 등록된 핸들러의 호출을 완료한 메시지 수.
     */
    std::size_t mDispatchedCount{};
    /**
     * @brief 대응하는 핸들러가 없어 폐기한 메시지 수.
     */
    std::size_t mUnhandledCount{};
    /**
     * @brief 이미 디스패치 중이어서 이번 호출을 지연 요청으로만 처리했는지 나타내는 상태.
     */
    bool mDeferred{false};
};

/**
 * @brief 메시지를 선입선출로 보관하고 타입별로 하나의 핸들러에 전달한다.
 * @details 디스패치 중 추가한 핸들러는 현재 순회가 끝난 뒤 등록한다. 재진입 Dispatch()는 즉시 순회하지 않고 외부 호출에 재처리를 요청한다.
 * @note 채널 내부에서는 스레드 동기화를 수행하지 않는다.
 */
class FMessageChannel {
public:
    /**
     * @brief 핸들러 등록과 디스패치 기능 없이 메시지 추가 기능만 제공하는 송신 인터페이스.
     * @details 채널을 소유하지 않으며 복사와 이동은 같은 채널 포인터를 보관한다.
     * @note 대상 채널은 이 송신 인터페이스를 사용하는 동안 유효해야 한다.
     */
    class FSender {
        friend class FMessageChannel;

    public:
        /**
         * @brief 지정한 채널로 메시지를 보낼 송신 인터페이스를 생성한다.
         * @param InChannel 메시지를 받을 채널.
         */
        explicit FSender(FMessageChannel& InChannel) noexcept;
        /**
         * @brief 송신 인터페이스를 소멸한다. 대상 채널은 소멸시키지 않는다.
         */
        ~FSender() noexcept = default;
        /**
         * @brief 같은 채널을 가리키는 송신 인터페이스를 복사 생성한다.
         */
        FSender(const FSender&) = default;
        /**
         * @brief 원본과 같은 채널을 가리키도록 복사 할당한다.
         */
        FSender& operator=(const FSender&) = default;
        /**
         * @brief 같은 채널 포인터를 가진 송신 인터페이스를 이동 생성한다. 원본 포인터도 유지된다.
         */
        FSender(FSender&&) = default;
        /**
         * @brief 원본과 같은 채널 포인터를 이동 할당한다. 원본 포인터도 유지된다.
         */
        FSender& operator=(FSender&&) = default;

    public:
        /**
         * @brief 대상 채널에 지정한 타입의 메시지를 생성하여 추가한다.
         * @tparam TMessage 생성할 메시지 타입.
         * @tparam Args 예외 없는 메시지 생성에 사용할 인수 타입들.
         * @param Arguments 메시지 생성자에 완벽 전달할 인수들.
         * @return 메시지를 추가하면 true, 채널 용량이 가득 차면 false.
         */
        template <CMessageType TMessage, typename... Args>
            requires CMessageConstructible<TMessage, Args...>
        bool TryEmplace(Args&&... Arguments);
        /**
         * @brief 기존 메시지 값을 대상 채널에 전달한다.
         * @tparam TMessage 참조와 cv 한정자를 제거한 타입이 CMessageType을 만족하며 전달한 값으로 예외 없이 생성 가능해야 한다.
         * @param Message 채널에 추가할 값. 값 범주에 따라 복사하거나 이동한다.
         * @return 메시지를 추가하면 true, 채널 용량이 가득 차면 false.
         */
        template <typename TMessage>
            requires CMessageType<std::remove_cvref_t<TMessage>>
        bool TryPush(TMessage&& Message);

    private:
        /**
         * @brief 소유권 없이 참조하는 대상 채널의 포인터.
         */
        FMessageChannel* mChannel{nullptr};
    };

public:
    /**
     * @brief 메시지 용량과 핸들러 예상 개수를 지정하여 채널을 생성한다.
     * @param InCapacity 보관할 메시지 수의 상한. 0이면 개수를 제한하지 않는다.
     * @param ExpectedHandlerCount 활성 및 대기 핸들러 배열에 미리 확보할 공간의 개수. 등록 개수의 상한은 아니다.
     */
    explicit FMessageChannel(std::size_t InCapacity = 0, std::size_t ExpectedHandlerCount = 8);

    /**
     * @brief 메시지 큐와 등록 상태를 가진 채널의 복사 생성을 금지한다.
     */
    FMessageChannel(const FMessageChannel&) = delete;
    /**
     * @brief 채널의 복사 할당을 금지한다.
     */
    FMessageChannel& operator=(const FMessageChannel&) = delete;

    /**
     * @brief 송신 인터페이스가 참조하는 채널의 이동 생성을 금지한다.
     */
    FMessageChannel(FMessageChannel&&) = delete;
    /**
     * @brief 채널의 이동 할당을 금지한다.
     */
    FMessageChannel& operator=(FMessageChannel&&) = delete;

public:
    /**
     * @brief 현재 채널의 메시지 추가 인터페이스를 생성한다.
     * @return 현재 채널을 소유권 없이 참조하는 송신 인터페이스.
     */
    [[nodiscard]] FSender GetSender() noexcept;

    /**
     * @brief 메시지 타입 하나에 호출 대상을 등록한다.
     * @tparam TMessage 처리할 메시지 타입.
     * @tparam TCallable 복사 생성 가능하고 const TMessage 참조로 호출 가능한 타입.
     * @param Callable 핸들러에 저장할 호출 대상.
     * @return 새로 등록하거나 지연 등록하면 true, 같은 타입이 이미 활성 또는 대기 목록에 있으면 false.
     * @details 디스패치 중에는 대기 목록에 추가하고 현재 순회가 끝난 뒤 활성 목록으로 옮긴다.
     */
    template <CMessageType TMessage, typename TCallable>
        requires std::copy_constructible<std::decay_t<TCallable>> && std::invocable<std::decay_t<TCallable>&, const TMessage&>
    bool TryBind(TCallable&& Callable);

    /**
     * @brief 지정한 타입의 메시지를 생성하여 큐 끝에 추가한다.
     * @tparam TMessage 생성할 메시지 타입.
     * @tparam Args 예외 없는 메시지 생성에 사용할 인수 타입들.
     * @param Arguments 메시지 생성자에 완벽 전달할 인수들.
     * @return 추가하면 true, 용량이 가득 차 생성하지 않으면 false.
     */
    template <CMessageType TMessage, typename... Args>
        requires CMessageConstructible<TMessage, Args...>
    bool TryEmplace(Args&&... Arguments);

    /**
     * @brief 기존 메시지 값을 큐 끝에 추가한다.
     * @tparam TMessage 참조와 cv 한정자를 제거한 타입이 CMessageType을 만족하며 전달한 값으로 예외 없이 생성 가능해야 한다.
     * @param Message 추가할 값. 값 범주에 따라 복사하거나 이동한다.
     * @return 추가하면 true, 용량이 가득 차면 false.
     */
    template <typename TMessage>
        requires CMessageType<std::remove_cvref_t<TMessage>>
    bool TryPush(TMessage&& Message);

    /**
     * @brief 대기 중인 메시지를 꺼내 등록된 핸들러에 전달한다.
     * @return 핸들러가 처리한 수, 핸들러가 없어 폐기한 수 및 이번 호출의 지연 여부.
     * @details 각 순회 시작 시의 메시지 수만 처리한다. 처리 중 추가한 메시지는 다음 호출까지 남지만 재진입 Dispatch() 요청이 있으면 같은 외부 호출에서 추가 순회한다.
     * @details 각 순회 끝에 대기 핸들러를 등록한다. 재진입 호출은 처리 수가 0이고 mDeferred가 true인 결과를 즉시 반환한다.
     */
    FMessageDispatchResult Dispatch();
    /**
     * @brief 큐의 모든 메시지를 소멸시켜 비운다.
     * @details 등록된 핸들러와 채널 용량은 유지한다.
     * @pre Dispatch()가 진행 중일 때는 호출하지 않는다.
     */
    void Clear() noexcept;

    /**
     * @brief 메시지 큐가 비어 있는지 확인한다.
     * @return 대기 메시지가 없으면 true.
     */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /**
     * @brief 큐가 설정한 메시지 수의 상한에 도달했는지 확인한다.
     * @return 용량이 0이 아니고 큐 크기가 용량 이상이면 true. 무제한 채널이면 false.
     */
    [[nodiscard]] bool IsFull() const noexcept;

    /**
     * @brief 큐에 남아 있는 메시지 수를 조회한다.
     * @return 아직 큐에서 꺼내지 않은 메시지 수.
     */
    [[nodiscard]] std::size_t Size() const noexcept;

    /**
     * @brief 메시지 수의 상한을 조회한다.
     * @return 생성 시 지정한 용량. 0이면 무제한.
     */
    [[nodiscard]] std::size_t GetCapacity() const noexcept;

private:
    /**
     * @brief 활성 목록에서 타입 정보에 대응하는 핸들러를 찾는다.
     * @param Type 조회할 메시지 타입 정보 포인터.
     * @return 대응하는 핸들러의 포인터, 없으면 nullptr.
     */
    FMessageHandler* FindHandler(const FMessageTypeInfo* Type) noexcept;
    /**
     * @brief 지연 등록 목록에서 타입 정보에 대응하는 핸들러를 찾는다.
     * @param Type 조회할 메시지 타입 정보 포인터.
     * @return 대응하는 대기 핸들러의 포인터, 없으면 nullptr.
     */
    FMessageHandler* FindPendingHandler(const FMessageTypeInfo* Type) noexcept;

    /**
     * @brief 대기 핸들러를 활성 목록으로 이동하고 대기 목록을 비운다.
     */
    void CommitPendingHandlers();

private:
    /**
     * @brief 아직 처리하지 않은 메시지를 소유하는 선입선출 큐.
     */
    std::deque<FMessage> mMessages{};
    /**
     * @brief 현재 디스패치에 사용할 활성 핸들러 목록.
     */
    std::vector<FMessageHandler> mHandlers{};
    /**
     * @brief 디스패치 중 등록 요청되어 현재 순회 종료를 기다리는 핸들러 목록.
     */
    std::vector<FMessageHandler> mPendingHandlers{};

    /**
     * @brief 큐의 메시지 수 상한. 0이면 무제한이다.
     */
    std::size_t mCapacity{};

    /**
     * @brief 외부 Dispatch() 호출의 순회가 진행 중인지 나타내는 상태.
     */
    bool mIsDispatching{false};
    /**
     * @brief 재진입 호출이 현재 순회 이후 추가 처리를 요청했는지 나타내는 상태.
     */
    bool mRedispatchRequested{false};
};

template <CMessageType TMessage, typename... Args>
    requires CMessageConstructible<TMessage, Args...>
bool FMessageChannel::FSender::TryEmplace(Args&&... Arguments) {
    return mChannel->TryEmplace<TMessage>(std::forward<Args>(Arguments)...);
}

template <typename TMessage>
    requires CMessageType<std::remove_cvref_t<TMessage>>
bool FMessageChannel::FSender::TryPush(TMessage&& Message) {
    using FMessageType = std::remove_cvref_t<TMessage>;

    return TryEmplace<FMessageType>(std::forward<TMessage>(Message));
}

template <CMessageType TMessage, typename TCallable>
    requires std::copy_constructible<std::decay_t<TCallable>> && std::invocable<std::decay_t<TCallable>&, const TMessage&>
bool FMessageChannel::TryBind(TCallable&& Callable) {
    const FMessageTypeInfo& Type{TMessage::StaticTypeInfo()};

    if (FindHandler(&Type) != nullptr || FindPendingHandler(&Type) != nullptr) {
        return false;
    }

    FMessageHandler Handler{FMessageHandler::Create<TMessage>(std::forward<TCallable>(Callable))};

    if (mIsDispatching) {
        mPendingHandlers.emplace_back(std::move(Handler));
        return true;
    }

    mHandlers.emplace_back(std::move(Handler));

    return true;
}

template <CMessageType TMessage, typename... Args>
    requires CMessageConstructible<TMessage, Args...>
bool FMessageChannel::TryEmplace(Args&&... Arguments) {
    if (IsFull()) {
        return false;
    }

    FMessage Message{};

    Message.TryEmplace<TMessage>(std::forward<Args>(Arguments)...);

    mMessages.emplace_back(std::move(Message));

    return true;
}

template <typename TMessage>
    requires CMessageType<std::remove_cvref_t<TMessage>>
bool FMessageChannel::TryPush(TMessage&& Message) {
    using FMessageType = std::remove_cvref_t<TMessage>;

    return TryEmplace<FMessageType>(std::forward<TMessage>(Message));
}
