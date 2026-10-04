#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "CoreUObject/UObjectSystem.h"

template <typename TSubsystem, typename TOwner>
class TSubsystemCollection {
public:
    TSubsystemCollection() = default;
    ~TSubsystemCollection();
    TSubsystemCollection(const TSubsystemCollection&) = delete;
    TSubsystemCollection& operator=(const TSubsystemCollection&) = delete;
    TSubsystemCollection(TSubsystemCollection&&) = delete;
    TSubsystemCollection& operator=(TSubsystemCollection&&) = delete;

public:
    void Initialize(TOwner& Owner);
    void Deinitialize();

    template <typename T>
        requires std::is_base_of_v<TSubsystem, T>
    T& Add();
    template <typename T>
        requires std::is_base_of_v<TSubsystem, T>
    T* Get() const;

private:
    TOwner* mOwner{};
    std::vector<std::unique_ptr<TSubsystem>> mSubsystems{};
    bool mDeinitializing{};
};

template <typename TSubsystem, typename TOwner>
TSubsystemCollection<TSubsystem, TOwner>::~TSubsystemCollection() {
    Deinitialize();
}

template <typename TSubsystem, typename TOwner>
void TSubsystemCollection<TSubsystem, TOwner>::Initialize(TOwner& Owner) {
    if (mOwner != nullptr && mOwner != &Owner) {
        throw std::logic_error{"Subsystem collection already has an owner"};
    }
    mOwner = &Owner;
}

template <typename TSubsystem, typename TOwner>
void TSubsystemCollection<TSubsystem, TOwner>::Deinitialize() {
    if (mDeinitializing) {
        return;
    }
    mDeinitializing = true;
    while (!mSubsystems.empty()) {
        TSubsystem& Subsystem{*mSubsystems.back()};
        Subsystem.Deinitialize();
        UObjectSystem::Unregister(&Subsystem, Subsystem.GetHandle());
        mSubsystems.pop_back();
    }
    mOwner = nullptr;
    mDeinitializing = false;
}

template <typename TSubsystem, typename TOwner>
template <typename T>
    requires std::is_base_of_v<TSubsystem, T>
T& TSubsystemCollection<TSubsystem, TOwner>::Add() {
    if (mOwner == nullptr || mDeinitializing) {
        throw std::logic_error{"Subsystem collection is not initialized"};
    }
    if (T* Existing{Get<T>()}) {
        return *Existing;
    }
    std::unique_ptr<T> Subsystem{std::make_unique<T>()};
    T* Result{Subsystem.get()};
    const std::size_t InitialCount{mSubsystems.size()};
    mSubsystems.push_back(std::move(Subsystem));
    try {
        UObjectSystem::Register(Result);
        Result->Initialize(mOwner);
    } catch (...) {
        mDeinitializing = true;
        while (mSubsystems.size() > InitialCount) {
            TSubsystem& FailedSubsystem{*mSubsystems.back()};
            FailedSubsystem.Deinitialize();
            UObjectSystem::Unregister(&FailedSubsystem, FailedSubsystem.GetHandle());
            mSubsystems.pop_back();
        }
        mDeinitializing = false;
        throw;
    }
    return *Result;
}

template <typename TSubsystem, typename TOwner>
template <typename T>
    requires std::is_base_of_v<TSubsystem, T>
T* TSubsystemCollection<TSubsystem, TOwner>::Get() const {
    for (const std::unique_ptr<TSubsystem>& Subsystem : mSubsystems) {
        if (T* Result{dynamic_cast<T*>(Subsystem.get())}) {
            return Result;
        }
    }
    return nullptr;
}
