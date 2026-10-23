/**
 * @file TSubsystemCollection.h
 * @brief 소유 객체에 연결된 서브시스템의 생성, 조회와 역순 종료를 관리합니다.
 */
#pragma once
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Core/Base/ErrorHandler.h"

#include <memory>
#include <type_traits>
#include <vector>
#include "CoreUObject/UObjectSystem.h"

/**
 * @brief 서브시스템 객체를 unique_ptr로 소유하고 등록 수명을 관리합니다.
 * @tparam TSubsystem 리플렉션과 Initialize, Deinitialize를 제공하는 서브시스템 기반 타입입니다.
 * @tparam TOwner UObject 기반의 소유 객체 타입입니다.
 */
template <typename TSubsystem, typename TOwner>
class TSubsystemCollection {
public:
    /**
     * @brief 소유 객체와 서브시스템이 없는 컬렉션을 생성합니다.
     */
    TSubsystemCollection() = default;
    /**
     * @brief Deinitialize로 서브시스템을 역순 종료하고 해제합니다.
     */
    ~TSubsystemCollection();
    /**
     * @brief 서브시스템 소유권 복사를 금지합니다.
     */
    TSubsystemCollection(const TSubsystemCollection&) = delete;
    /**
     * @brief 서브시스템 소유권의 복사 대입을 금지합니다.
     */
    TSubsystemCollection& operator=(const TSubsystemCollection&) = delete;
    /**
     * @brief 소유 객체 연결을 유지하기 위해 이동 생성을 금지합니다.
     */
    TSubsystemCollection(TSubsystemCollection&&) = delete;
    /**
     * @brief 소유 객체 연결을 유지하기 위해 이동 대입을 금지합니다.
     */
    TSubsystemCollection& operator=(TSubsystemCollection&&) = delete;

public:
    /**
     * @brief 서브시스템을 추가할 소유 객체를 설정합니다.
     * @param Owner 서브시스템의 Outer로 사용할 객체입니다.
     * @note 다른 소유 객체가 이미 설정되었으면 Critical 오류를 보고합니다.
     */
    void Initialize(TOwner& Owner);
    /**
     * @brief 추가 역순으로 종료, 등록 해제와 객체 삭제를 수행하고 소유 객체를 비웁니다.
     * @note 종료 중 재진입하면 아무 작업도 하지 않습니다.
     */
    void Deinitialize();

    /**
     * @brief T와 호환되는 기존 서브시스템을 반환하거나 새로 생성해 등록·초기화합니다.
     * @tparam T TSubsystem 파생 타입입니다.
     * @pre 컬렉션이 초기화되어 있고 종료 중이 아니어야 합니다.
     * @return T 호환 서브시스템의 참조입니다.
     * @note 새 객체를 컬렉션에 저장한 뒤 Initialize를 호출합니다.
     */
    template <typename T>
        requires std::is_base_of_v<TSubsystem, T>
    T& Add();
    /**
     * @brief 추가 순서대로 T와 호환되는 첫 서브시스템을 찾습니다.
     * @tparam T TSubsystem 파생 타입입니다.
     * @return 첫 일치 객체입니다. 없으면 nullptr입니다.
     */
    template <typename T>
        requires std::is_base_of_v<TSubsystem, T>
    T* Get() const;

private:
    /**
     * @brief 서브시스템의 Outer로 사용하는 비소유 객체 포인터입니다.
     */
    TOwner* mOwner{};
    /**
     * @brief 추가 순서로 서브시스템의 소유권을 저장한 목록입니다.
     */
    std::vector<std::unique_ptr<TSubsystem>> mSubsystems{};
    /**
     * @brief 종료 재진입과 종료 중 추가를 차단하는 상태입니다.
     */
    bool mDeinitializing{};
};

template <typename TSubsystem, typename TOwner>
TSubsystemCollection<TSubsystem, TOwner>::~TSubsystemCollection() {
    Deinitialize();
}

template <typename TSubsystem, typename TOwner>
void TSubsystemCollection<TSubsystem, TOwner>::Initialize(TOwner& Owner) {
    if (mOwner != nullptr && mOwner != &Owner) {
        ErrorHandler::Report("TSubsystemCollection", "Subsystem collection already has an owner", ErrorHandler::EErrorLevel::Critical);
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
        ErrorHandler::Report("TSubsystemCollection", "Subsystem collection is not initialized", ErrorHandler::EErrorLevel::Critical);
    }

    if (T * Existing{Get<T>()}) {
        return *Existing;
    }

    std::unique_ptr<T> Subsystem{NewObject<T>()};
    T* Result{Subsystem.get()};

    Result->SetOuter(mOwner);
    ErrorHandler::Report(!UObjectSystem::Register(Result).IsValid(), "TSubsystemCollection", "Cannot register subsystem", ErrorHandler::EErrorLevel::Critical);
    mSubsystems.push_back(std::move(Subsystem));
    Result->Initialize(mOwner);

    return *Result;
}

template <typename TSubsystem, typename TOwner>
template <typename T>
    requires std::is_base_of_v<TSubsystem, T>
T* TSubsystemCollection<TSubsystem, TOwner>::Get() const {
    for (const std::unique_ptr<TSubsystem>& Subsystem : mSubsystems) {
        if (T * Result{Cast<T>(Subsystem.get())}) {
            return Result;
        }
    }

    return nullptr;
}
