/**
 * @file TDelegate.h
 * @brief 여러 호출 함수를 등록해 한 번에 호출하고, UObject 구독자의 생존을 세대 핸들로 확인하는 델리게이트입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/UObject.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace CoreUObject {

    /**
     * @brief 델리게이트 구독을 해제할 때 사용하는 등록 번호입니다. 0은 등록 실패를 뜻합니다.
     */
    using FDelegateHandle = Core::Uint64;

    /**
     * @brief 다음에 발급할 델리게이트 등록 번호입니다. 한 번 발급한 번호는 다시 사용하지 않습니다.
     */
    inline CoreUObject::FDelegateHandle GNextDelegateHandle{1};

    /**
     * @brief 여러 호출 함수를 등록해 한 번에 호출하는 멀티캐스트 델리게이트입니다.
     * @tparam Args 호출 함수에 전달할 인수 타입입니다.
     * @note AddDynamic으로 등록한 UObject 구독은 호출 직전에 세대 핸들로 생존을 확인하고, 파괴된 객체의 구독은 자동으로 제거합니다.
     */
    template <typename... Args>
    class TDelegate {
    public:
        /**
         * @brief 등록 함수의 호출 시그니처입니다.
         */
        using HandlerType = std::function<void(Args...)>;

    public:
        /**
         * @brief 등록된 함수가 없는 델리게이트를 구성합니다.
         */
        TDelegate() = default;

        /**
         * @brief 등록 목록을 모두 해제합니다.
         */
        ~TDelegate();

        /**
         * @brief 구독 소유권이 모호해지지 않도록 복사 생성을 금지합니다.
         */
        TDelegate(const CoreUObject::TDelegate<Args...>&) = delete;

        /**
         * @brief 구독 소유권이 모호해지지 않도록 대입을 금지합니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        CoreUObject::TDelegate<Args...>& operator=(const CoreUObject::TDelegate<Args...>&) = delete;

    public:
        /**
         * @brief 일반 함수나 람다를 등록합니다.
         * @param[in] Handler 등록할 호출 함수입니다.
         * @return 해제에 사용할 등록 번호이며, 함수가 비어 있으면 0입니다.
         * @note 람다가 캡처한 객체의 생존은 확인하지 않습니다.
         */
        CoreUObject::FDelegateHandle Add(HandlerType Handler);

        /**
         * @brief UObject 파생 객체의 멤버 함수를 등록합니다.
         * @tparam T 멤버 함수를 가진 UObject 파생 타입입니다.
         * @param[in] Instance 멤버 함수의 대상 객체입니다.
         * @param[in] Func 등록할 멤버 함수 포인터입니다.
         * @return 해제에 사용할 등록 번호이며, 객체가 등록되지 않았거나 인수가 비어 있으면 0입니다.
         * @note 대상 객체가 파괴되면 호출하지 않고 구독을 제거합니다.
         */
        template <typename T>
        CoreUObject::FDelegateHandle AddDynamic(T* Instance, void (T::*Func)(Args...));

        /**
         * @brief UObject가 아닌 객체의 멤버 함수를 등록합니다.
         * @tparam T 멤버 함수를 가진 타입이며 UObject 파생 타입이 아니어야 합니다.
         * @param[in] Instance 멤버 함수의 대상 객체이며 구독을 해제할 때까지 살아 있어야 합니다.
         * @param[in] Func 등록할 멤버 함수 포인터입니다.
         * @return 해제에 사용할 등록 번호이며, 인수가 비어 있으면 0입니다.
         */
        template <typename T>
        CoreUObject::FDelegateHandle AddRaw(T* Instance, void (T::*Func)(Args...));

        /**
         * @brief 등록 번호에 해당하는 구독을 해제합니다.
         * @param[in] Handle 해제할 등록 번호입니다.
         */
        void Remove(CoreUObject::FDelegateHandle Handle);

        /**
         * @brief 지정 객체가 AddDynamic 또는 AddRaw로 등록한 구독을 모두 해제합니다.
         * @tparam T 구독 대상 객체의 타입입니다.
         * @param[in] Instance 구독을 해제할 객체입니다.
         */
        template <typename T>
        void RemoveAll(const T* Instance);

        /**
         * @brief 현재 호출할 수 있는 구독이 하나라도 있는지 검사합니다.
         * @return 호출 가능한 구독이 있으면 true, 그렇지 않으면 false입니다.
         */
        bool IsBound() const;

        /**
         * @brief 등록 목록의 사본에 있는 모든 함수를 인수와 함께 호출합니다.
         * @param[in] Arguments 등록된 함수에 전달할 인수입니다.
         * @note 호출 도중 등록·해제가 일어나도 안전하며, 해제된 구독은 이후 호출하지 않습니다.
         */
        void Broadcast(Args... Arguments);

        /**
         * @brief 등록된 구독을 모두 해제합니다.
         */
        void Clear();

    private:
        /**
         * @brief 등록 번호와 호출 함수, 구독자 식별 정보를 보관하는 구독 항목입니다.
         */
        struct FHandler {
        public:
            /**
             * @brief 구독의 등록 번호입니다.
             */
            CoreUObject::FDelegateHandle mHandle{};

            /**
             * @brief 호출할 함수입니다.
             */
            HandlerType mHandler{};

            /**
             * @brief RemoveAll에서 AddRaw 구독자를 구별할 비소유 주소입니다.
             */
            const void* mInstance{};

            /**
             * @brief AddDynamic 구독자의 세대 핸들이며, UObject 구독이 아니면 유효하지 않습니다.
             */
            CoreUObject::FObjectHandle mObject{};

            /**
             * @brief 해제되지 않은 구독인지 여부입니다.
             */
            bool mActive{true};
        };

    private:
        /**
         * @brief 구독 항목을 만들고 새 등록 번호를 발급합니다.
         * @param[in] Handler 등록할 호출 함수입니다.
         * @param[in] Instance AddRaw 구독자의 주소이며, 없으면 nullptr입니다.
         * @param[in] Object AddDynamic 구독자의 세대 핸들입니다.
         * @return 발급한 등록 번호이며, 함수가 비어 있으면 0입니다.
         */
        CoreUObject::FDelegateHandle AddInternal(HandlerType Handler, const void* Instance, CoreUObject::FObjectHandle Object);

        /**
         * @brief 구독이 해제되지 않았고 UObject 구독자가 살아 있는지 검사합니다.
         * @param[in] Entry 검사할 구독 항목입니다.
         * @return 호출할 수 있으면 true, 그렇지 않으면 false입니다.
         */
        static bool IsCallable(const FHandler& Entry);

        /**
         * @brief 해제된 구독 항목을 등록 목록에서 제거합니다.
         */
        void RemoveInactive();

    private:
        /**
         * @brief 등록 순서대로 보관하는 구독 항목입니다. Broadcast 사본과 해제 상태를 공유하도록 공유 포인터로 보관합니다.
         */
        Core::TArray<std::shared_ptr<FHandler>> mHandlers{};
    };

    /**
     * @brief 등록 목록을 모두 해제합니다.
     */
    template <typename... Args>
    CoreUObject::TDelegate<Args...>::~TDelegate() {
        Clear();
    }

    /**
     * @brief 일반 함수나 람다를 등록합니다.
     */
    template <typename... Args>
    CoreUObject::FDelegateHandle CoreUObject::TDelegate<Args...>::Add(HandlerType Handler) {
        return AddInternal(std::move(Handler), nullptr, CoreUObject::FObjectHandle{});
    }

    /**
     * @brief UObject 파생 객체의 멤버 함수를 등록합니다.
     */
    template <typename... Args>
    template <typename T>
    CoreUObject::FDelegateHandle CoreUObject::TDelegate<Args...>::AddDynamic(T* Instance, void (T::*Func)(Args...)) {
        static_assert(std::is_base_of_v<CoreUObject::UObject, T>, "AddDynamic requires a UObject-derived class.");

        if (Instance == nullptr || Func == nullptr) {
            return 0;
        }

        const CoreUObject::FObjectHandle Object{Instance->GetHandle()};

        if (CoreUObject::FObjectRegistry::Get()->Resolve(Object) != Instance) {
            return 0;
        }

        return AddInternal([Instance, Func](Args... Arguments) { (Instance->*Func)(Arguments...); }, nullptr, Object);
    }

    /**
     * @brief UObject가 아닌 객체의 멤버 함수를 등록합니다.
     */
    template <typename... Args>
    template <typename T>
    CoreUObject::FDelegateHandle CoreUObject::TDelegate<Args...>::AddRaw(T* Instance, void (T::*Func)(Args...)) {
        static_assert(!std::is_base_of_v<CoreUObject::UObject, T>, "Use AddDynamic for UObject-derived classes.");

        if (Instance == nullptr || Func == nullptr) {
            return 0;
        }

        return AddInternal([Instance, Func](Args... Arguments) { (Instance->*Func)(Arguments...); }, Instance, CoreUObject::FObjectHandle{});
    }

    /**
     * @brief 등록 번호에 해당하는 구독을 해제합니다.
     */
    template <typename... Args>
    void CoreUObject::TDelegate<Args...>::Remove(CoreUObject::FDelegateHandle Handle) {
        if (Handle == 0) {
            return;
        }

        for (const auto& Entry : mHandlers) {
            if (Entry->mHandle == Handle) {
                Entry->mActive = false;
                break;
            }
        }

        RemoveInactive();
    }

    /**
     * @brief 지정 객체가 AddDynamic 또는 AddRaw로 등록한 구독을 모두 해제합니다.
     */
    template <typename... Args>
    template <typename T>
    void CoreUObject::TDelegate<Args...>::RemoveAll(const T* Instance) {
        if (Instance == nullptr) {
            return;
        }

        for (const auto& Entry : mHandlers) {
            if constexpr (std::is_base_of_v<CoreUObject::UObject, T>) {
                if (Entry->mObject.IsValid() && Entry->mObject == Instance->GetHandle()) {
                    Entry->mActive = false;
                }
            } else {
                if (Entry->mInstance == Instance) {
                    Entry->mActive = false;
                }
            }
        }

        RemoveInactive();
    }

    /**
     * @brief 현재 호출할 수 있는 구독이 하나라도 있는지 검사합니다.
     */
    template <typename... Args>
    bool CoreUObject::TDelegate<Args...>::IsBound() const {
        return std::any_of(mHandlers.begin(), mHandlers.end(), [](const std::shared_ptr<FHandler>& Entry) { return IsCallable(*Entry); });
    }

    /**
     * @brief 등록 목록의 사본에 있는 모든 함수를 인수와 함께 호출합니다.
     */
    template <typename... Args>
    void CoreUObject::TDelegate<Args...>::Broadcast(Args... Arguments) {
        const Core::TArray<std::shared_ptr<FHandler>> Snapshot{mHandlers};

        for (const auto& Entry : Snapshot) {
            if (!IsCallable(*Entry)) {
                Entry->mActive = false;
                continue;
            }

            Entry->mHandler(Arguments...);
        }

        RemoveInactive();
    }

    /**
     * @brief 등록된 구독을 모두 해제합니다.
     */
    template <typename... Args>
    void CoreUObject::TDelegate<Args...>::Clear() {
        for (const auto& Entry : mHandlers) {
            Entry->mActive = false;
        }

        mHandlers.clear();
    }

    /**
     * @brief 구독 항목을 만들고 새 등록 번호를 발급합니다.
     */
    template <typename... Args>
    CoreUObject::FDelegateHandle CoreUObject::TDelegate<Args...>::AddInternal(HandlerType Handler, const void* Instance, CoreUObject::FObjectHandle Object) {
        if (!Handler) {
            return 0;
        }

        auto Entry{std::make_shared<FHandler>()};

        Entry->mHandle = GNextDelegateHandle++;
        Entry->mHandler = std::move(Handler);
        Entry->mInstance = Instance;
        Entry->mObject = Object;

        const CoreUObject::FDelegateHandle Handle{Entry->mHandle};

        mHandlers.push_back(std::move(Entry));

        return Handle;
    }

    /**
     * @brief 구독이 해제되지 않았고 UObject 구독자가 살아 있는지 검사합니다.
     */
    template <typename... Args>
    bool CoreUObject::TDelegate<Args...>::IsCallable(const FHandler& Entry) {
        if (!Entry.mActive) {
            return false;
        }

        // 일반 함수·람다·AddRaw 구독은 대상의 생존을 사용하는 쪽이 보장합니다.
        if (!Entry.mObject.IsValid()) {
            return true;
        }

        // 슬롯이 재사용되었더라도 세대가 다르면 nullptr이므로 다른 객체를 호출하지 않습니다.
        return CoreUObject::FObjectRegistry::Get()->Resolve(Entry.mObject) != nullptr;
    }

    /**
     * @brief 해제된 구독 항목을 등록 목록에서 제거합니다.
     */
    template <typename... Args>
    void CoreUObject::TDelegate<Args...>::RemoveInactive() {
        mHandlers.erase(std::remove_if(mHandlers.begin(), mHandlers.end(), [](const std::shared_ptr<FHandler>& Entry) { return !Entry->mActive; }), mHandlers.end());
    }

}
