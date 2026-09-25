#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

template <typename T>
class FStateChannel {
public:
    using VersionType = std::uint64_t;

    struct FReadResult {
        const T* mValue{nullptr};
        bool mChanged{false};
    };

public:
    class FReader {
    public:
        explicit FReader(const FStateChannel& InChannel) noexcept;
        FReader() noexcept = default;
        ~FReader() noexcept = default;
        FReader(const FReader&) = default;
        FReader& operator=(const FReader&) = default;
        FReader(FReader&&) = default;
        FReader& operator=(FReader&&) = default;

    public:
        [[nodiscard]] bool HasChanged() const noexcept;
        [[nodiscard]] bool HasValue() const noexcept;
        [[nodiscard]] const T& Peek() const;
        [[nodiscard]] const T& Read();
        [[nodiscard]] FReadResult ReadIfChanged() noexcept;

    private:
        const FStateChannel* mChannel{nullptr};
        VersionType mLastReadVersion{0};
    };

    class FWriter {
    public:
        explicit FWriter(FStateChannel& InChannel) noexcept;
        FWriter() noexcept = default;
        ~FWriter() noexcept = default;
        FWriter(const FWriter&) = default;
        FWriter& operator=(const FWriter&) = default;
        FWriter(FWriter&&) = default;
        FWriter& operator=(FWriter&&) = default;

    public:
        void Write(const T& NewState) requires std::copy_constructible<T> && std::assignable_from<T&, const T&>;
        void Write(T&& NewState) requires std::move_constructible<T> && std::assignable_from<T&, T>;
        template <typename... Args> requires std::constructible_from<T, Args...> const T& Emplace(Args&&... Arguments);
        template <typename TCallable> requires std::invocable<TCallable&, T&> bool Modify(TCallable&& Callable);
        void Clear() noexcept;
        [[nodiscard]] bool HasValue() const noexcept;

    private:
        FStateChannel<T>* mChannel{nullptr};
    };

    class FReadWriter {
    public:
        explicit FReadWriter(FStateChannel& InChannel) noexcept;
        explicit FReadWriter() noexcept = default;
        ~FReadWriter() noexcept = default;
        FReadWriter(const FReadWriter&) = default;
        FReadWriter& operator=(const FReadWriter&) = default;
        FReadWriter(FReadWriter&&) = default;
        FReadWriter& operator=(FReadWriter&&) = default;

    public:
        [[nodiscard]] bool HasChanged() const noexcept;
        [[nodiscard]] bool HasValue() const noexcept;
        [[nodiscard]] const T& Peek() const;
        [[nodiscard]] const T& Read();
        [[nodiscard]] FReadResult ReadIfChanged() noexcept;
        void Write(const T& NewState) requires std::copy_constructible<T> && std::assignable_from<T&, const T&>;
        void Write(T&& NewState) requires std::move_constructible<T> && std::assignable_from<T&, T>;
        template <typename... Args> requires std::constructible_from<T, Args...> const T& Emplace(Args&&... Arguments);
        template <typename TCallable> requires std::invocable<TCallable&, T&> bool Modify(TCallable&& Callable);
        void Clear() noexcept;

    private:
        FStateChannel<T>* mChannel{nullptr};
        VersionType mLastReadVersion{0};
    };

public:
    FStateChannel() = default;
    explicit FStateChannel(const T& InitialState) requires std::copy_constructible<T>;
    explicit FStateChannel(T&& InitialState) requires std::move_constructible<T>;
    template <typename... Args> requires std::constructible_from<T, Args...> explicit FStateChannel(std::in_place_t, Args&&... Arguments);
    FStateChannel(const FStateChannel&) = delete;
    FStateChannel& operator=(const FStateChannel&) = delete;
    FStateChannel(FStateChannel&&) = delete;
    FStateChannel& operator=(FStateChannel&&) = delete;

public:
    [[nodiscard]] FReader GetReader() const noexcept;
    [[nodiscard]] FWriter GetWriter() noexcept;
    [[nodiscard]] FReadWriter GetReadWriter() noexcept;

private:
    void Write(const T& NewState) requires std::copy_constructible<T> && std::assignable_from<T&, const T&>;
    void Write(T&& NewState) requires std::move_constructible<T> && std::assignable_from<T&, T>;
    template <typename... Args> requires std::constructible_from<T, Args...> const T& Emplace(Args&&... Arguments);
    template <typename TCallable> requires std::invocable<TCallable&, T&> bool TryModify(TCallable&& Callable);
    void Clear() noexcept;

private:
    std::optional<T> mState{};
    VersionType mVersion{0};
};

template <typename T> FStateChannel<T>::FReader::FReader(const FStateChannel& InChannel) noexcept
    : mChannel(&InChannel) {
}

template <typename T> [[nodiscard]] bool FStateChannel<T>::FReader::HasChanged() const noexcept {
    return mLastReadVersion != mChannel->mVersion;
}

template <typename T> [[nodiscard]] bool FStateChannel<T>::FReader::HasValue() const noexcept {
    return mChannel->mState.has_value();
}

template <typename T> [[nodiscard]] const T& FStateChannel<T>::FReader::Peek() const {
    return mChannel->mState.value();
}

template <typename T> [[nodiscard]] const T& FStateChannel<T>::FReader::Read() {
    const T& Value{mChannel->mState.value()};
    mLastReadVersion = mChannel->mVersion;

    return Value;
}

template <typename T> [[nodiscard]] typename FStateChannel<T>::FReadResult FStateChannel<T>::FReader::ReadIfChanged() noexcept {
    if (!HasChanged()) {
        return {};
    }

    mLastReadVersion = mChannel->mVersion;

    return {.mValue = mChannel->mState ? std::addressof(*mChannel->mState) : nullptr, .mChanged = true};
}

template <typename T> FStateChannel<T>::FWriter::FWriter(FStateChannel& InChannel) noexcept
    : mChannel(&InChannel) {
}

template <typename T> void FStateChannel<T>::FWriter::Write(const T& NewState) requires std::copy_constructible<T> && std::assignable_from<T&, const T&> {
    mChannel->Write(NewState);
}

template <typename T> void FStateChannel<T>::FWriter::Write(T&& NewState) requires std::move_constructible<T> && std::assignable_from<T&, T> {
    mChannel->Write(std::move(NewState));
}

template <typename T> template <typename... Args> requires std::constructible_from<T, Args...> const T& FStateChannel<T>::FWriter::Emplace(Args&&... Arguments) {
    return mChannel->Emplace(std::forward<Args>(Arguments)...);
}

template <typename T> template <typename TCallable> requires std::invocable<TCallable&, T&> bool FStateChannel<T>::FWriter::Modify(TCallable&& Callable) {
    return mChannel->TryModify(std::forward<TCallable>(Callable));
}

template <typename T> void FStateChannel<T>::FWriter::Clear() noexcept {
    mChannel->Clear();
}

template <typename T> [[nodiscard]] bool FStateChannel<T>::FWriter::HasValue() const noexcept {
    return mChannel->mState.has_value();
}

template <typename T> FStateChannel<T>::FReadWriter::FReadWriter(FStateChannel& InChannel) noexcept
    : mChannel(&InChannel) {
}

template <typename T> [[nodiscard]] bool FStateChannel<T>::FReadWriter::HasChanged() const noexcept {
    return mLastReadVersion != mChannel->mVersion;
}

template <typename T> [[nodiscard]] bool FStateChannel<T>::FReadWriter::HasValue() const noexcept {
    return mChannel->mState.has_value();
}

template <typename T> [[nodiscard]] const T& FStateChannel<T>::FReadWriter::Peek() const {
    return mChannel->mState.value();
}

template <typename T> [[nodiscard]] const T& FStateChannel<T>::FReadWriter::Read() {
    const T& Value{mChannel->mState.value()};
    mLastReadVersion = mChannel->mVersion;

    return Value;
}

template <typename T> [[nodiscard]] typename FStateChannel<T>::FReadResult FStateChannel<T>::FReadWriter::ReadIfChanged() noexcept {
    if (!HasChanged()) {
        return {};
    }

    mLastReadVersion = mChannel->mVersion;

    return {.mValue = mChannel->mState ? std::addressof(*mChannel->mState) : nullptr, .mChanged = true};
}

template <typename T> void FStateChannel<T>::FReadWriter::Write(const T& NewState) requires std::copy_constructible<T> && std::assignable_from<T&, const T&> {
    mChannel->Write(NewState);
}

template <typename T> void FStateChannel<T>::FReadWriter::Write(T&& NewState) requires std::move_constructible<T> && std::assignable_from<T&, T> {
    mChannel->Write(std::move(NewState));
}

template <typename T> template <typename... Args> requires std::constructible_from<T, Args...> const T& FStateChannel<T>::FReadWriter::Emplace(Args&&... Arguments) {
    return mChannel->Emplace(std::forward<Args>(Arguments)...);
}

template <typename T> template <typename TCallable> requires std::invocable<TCallable&, T&> bool FStateChannel<T>::FReadWriter::Modify(TCallable&& Callable) {
    return mChannel->TryModify(std::forward<TCallable>(Callable));
}

template <typename T> void FStateChannel<T>::FReadWriter::Clear() noexcept {
    mChannel->Clear();
}

template <typename T> FStateChannel<T>::FStateChannel(const T& InitialState) requires std::copy_constructible<T>
    : mState(InitialState),
      mVersion(1) {
}

template <typename T> FStateChannel<T>::FStateChannel(T&& InitialState) requires std::move_constructible<T>
    : mState(std::move(InitialState)),
      mVersion(1) {
}

template <typename T> template <typename... Args> requires std::constructible_from<T, Args...> FStateChannel<T>::FStateChannel(std::in_place_t, Args&&... Arguments)
    : mState(std::in_place, std::forward<Args>(Arguments)...),
      mVersion(1) {
}

template <typename T> [[nodiscard]] typename FStateChannel<T>::FReader FStateChannel<T>::GetReader() const noexcept {
    return FReader{*this};
}

template <typename T> [[nodiscard]] typename FStateChannel<T>::FWriter FStateChannel<T>::GetWriter() noexcept {
    return FWriter{*this};
}

template <typename T> [[nodiscard]] typename FStateChannel<T>::FReadWriter FStateChannel<T>::GetReadWriter() noexcept {
    return FReadWriter{*this};
}

template <typename T> void FStateChannel<T>::Write(const T& NewState) requires std::copy_constructible<T> && std::assignable_from<T&, const T&> {
    if (mState) {
        *mState = NewState;
    } else {
        mState.emplace(NewState);
    }

    ++mVersion;
}

template <typename T> void FStateChannel<T>::Write(T&& NewState) requires std::move_constructible<T> && std::assignable_from<T&, T> {
    if (mState) {
        *mState = std::move(NewState);
    } else {
        mState.emplace(std::move(NewState));
    }

    ++mVersion;
}

template <typename T> template <typename... Args> requires std::constructible_from<T, Args...> const T& FStateChannel<T>::Emplace(Args&&... Arguments) {
    mState.emplace(std::forward<Args>(Arguments)...);
    ++mVersion;

    return *mState;
}

template <typename T> template <typename TCallable> requires std::invocable<TCallable&, T&> bool FStateChannel<T>::TryModify(TCallable&& Callable) {
    if (!mState) {
        return false;
    }

    std::invoke(std::forward<TCallable>(Callable), *mState);
    ++mVersion;

    return true;
}

template <typename T> void FStateChannel<T>::Clear() noexcept {
    if (!mState) {
        return;
    }

    mState.reset();
    ++mVersion;
}
