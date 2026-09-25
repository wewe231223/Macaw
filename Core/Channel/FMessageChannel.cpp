#include "pch.h"
#include "FMessageChannel.h"

FMessageChannel::FSender FMessageChannel::GetSender() noexcept {
    return FMessageChannel::FSender{*this};
}

FMessageDispatchResult FMessageChannel::Dispatch() {
    if (mIsDispatching) {
        mRedispatchRequested = true;
        return {.mDeferred = true};
    }

    struct FDispatchScope {
        explicit FDispatchScope(bool& InIsDispatching) noexcept
            : mIsDispatching(InIsDispatching) {
            mIsDispatching = true;
        }

        ~FDispatchScope() noexcept {
            mIsDispatching = false;
        }

        bool& mIsDispatching;
    };

    FDispatchScope Scope{mIsDispatching};
    FMessageDispatchResult Result{};

    do {
        mRedispatchRequested = false;

        const std::size_t DispatchCount{mMessages.size()};

        for (std::size_t Index{0}; Index < DispatchCount; ++Index) {
            FMessage Message{std::move(mMessages.front())};
            mMessages.pop_front();

            FMessageHandler* Handler{FindHandler(Message.GetTypeInfo())};

            if (Handler == nullptr) {
                ++Result.mUnhandledCount;
                continue;
            }

            Handler->Invoke(Message);
            ++Result.mDispatchedCount;
        }

        CommitPendingHandlers();
    } while (mRedispatchRequested);

    return Result;
}

void FMessageChannel::Clear() noexcept {
    mMessages.clear();
}

bool FMessageChannel::IsEmpty() const noexcept {
    return mMessages.empty();
}

bool FMessageChannel::IsFull() const noexcept {
    return mCapacity != 0 && mMessages.size() >= mCapacity;
}

std::size_t FMessageChannel::Size() const noexcept {
    return mMessages.size();
}

std::size_t FMessageChannel::GetCapacity() const noexcept {
    return mCapacity;
}

FMessageHandler* FMessageChannel::FindHandler(const FTypeInfo* Type) noexcept {
    const auto Iterator{std::ranges::find_if(mHandlers, [&Type](const FMessageHandler& Handler) {
        return Handler.Handles(Type);
    })};

    return Iterator != mHandlers.end() ? &(*Iterator) : nullptr;
}

FMessageHandler* FMessageChannel::FindPendingHandler(const FTypeInfo* Type) noexcept {
    const auto Iterator{std::ranges::find_if(mPendingHandlers, [&Type](const FMessageHandler& Handler) {
        return Handler.Handles(Type);
    })};

    return Iterator != mPendingHandlers.end() ? &(*Iterator) : nullptr;
}

void FMessageChannel::CommitPendingHandlers() {
    if (mPendingHandlers.empty()) {
        return;
    }

    mHandlers.reserve(mHandlers.size() + mPendingHandlers.size());

    for (FMessageHandler& Handler : mPendingHandlers) {
        mHandlers.emplace_back(std::move(Handler));
    }

    mPendingHandlers.clear();
}

FMessageChannel::FSender::FSender(FMessageChannel& InChannel) noexcept
    : mChannel(&InChannel) {
}

FMessageChannel::FMessageChannel(std::size_t InCapacity, std::size_t ExpectedHandlerCount)
    : mCapacity(InCapacity) {
    mHandlers.reserve(ExpectedHandlerCount);
    mPendingHandlers.reserve(ExpectedHandlerCount);
}
