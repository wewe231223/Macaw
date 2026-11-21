/**
 * @file TObjectRef.h
 * @brief 객체를 소유하지 않고 세대 핸들과 GUID를 보관하는 형식 지정 객체 참조입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/Guid.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/UObject.h"

#include <cstddef>
#include <memory>
#include <type_traits>

namespace CoreUObject {

    class UClass;
    struct FObjectReference;

    /**
     * @brief 형식 지정 객체 참조의 레지스트리 해석을 지원하는 내부 함수입니다.
     */
    namespace ObjectRefPrivate {
        /**
         * @brief 세대 핸들을 해석하고 요청한 리플렉션 클래스와의 호환성을 확인합니다.
         * @param[in] Handle 해석하거나 해시할 런타임 객체 세대 핸들입니다.
         * @param[in] Class 참조 대상이 호환되어야 하는 리플렉션 클래스입니다.
         * @return 살아 있는 호환 객체의 비소유 주소이며 해석에 실패하면 nullptr입니다.
         */
        CoreUObject::UObject* Resolve(CoreUObject::FObjectHandle Handle, const CoreUObject::UClass* Class);

        /**
         * @brief 현재 핸들이 가리키는 객체의 GUID를 조회합니다.
         * @param[in] Handle 해석하거나 해시할 런타임 객체 세대 핸들입니다.
         * @return 해석한 객체의 GUID이며 유효하지 않으면 널 GUID입니다.
         */
        Core::FGuid GetGuid(CoreUObject::FObjectHandle Handle);
    }

    /**
     * @brief 객체를 소유하지 않고 세대 핸들과 GUID를 보관하는 형식 지정 객체 참조입니다.
     * @tparam T 참조 대상의 형식입니다.     */
    template <typename T>
    class TObjectRef {
    public:
        /**
         * @brief 객체 참조가 가리키는 대상 형식입니다.
         */
        using FObjectType = T;

    public:
        /**
         * @brief 대상이 없는 빈 객체 참조를 구성합니다.
         */
        TObjectRef() = default;

        /**
         * @brief 대상이 없는 빈 객체 참조를 구성합니다.
         */
        TObjectRef(std::nullptr_t);

        /**
         * @brief 대상 객체의 핸들과 GUID로 비소유 참조를 만듭니다.
         * @param[in] Object 비소유 참조를 만들거나 비교할 객체입니다.
         */
        TObjectRef(T* Object);

        /**
         * @brief 참조 저장 상태의 수명을 끝내며 대상 객체는 파괴하지 않습니다.
         */
        ~TObjectRef() = default;

        /**
         * @brief 대상 객체를 소유하지 않고 상대 참조의 핸들과 GUID를 복사하거나 이동합니다.
         * @param[in] Other 복사·이동 또는 비교할 상대 참조입니다.
         */
        TObjectRef(const CoreUObject::TObjectRef<T>& Other) = default;

        /**
         * @brief 대상 객체의 수명에 영향을 주지 않고 참조 상태를 복사하거나 이동합니다.
         * @param[in] Other 복사·이동 또는 비교할 상대 참조입니다.
         * @return 갱신한 자기 자신의 참조입니다.
         */
        CoreUObject::TObjectRef<T>& operator=(const CoreUObject::TObjectRef<T>& Other) = default;

        /**
         * @brief 대상 객체를 소유하지 않고 상대 참조의 핸들과 GUID를 복사하거나 이동합니다.
         * @param[in] Other 복사·이동 또는 비교할 상대 참조입니다.
         */
        TObjectRef(CoreUObject::TObjectRef<T>&& Other) noexcept = default;

        /**
         * @brief 대상 객체의 수명에 영향을 주지 않고 참조 상태를 복사하거나 이동합니다.
         * @param[in] Other 복사·이동 또는 비교할 상대 참조입니다.
         * @return 갱신한 자기 자신의 참조입니다.
         */
        CoreUObject::TObjectRef<T>& operator=(CoreUObject::TObjectRef<T>&& Other) noexcept = default;

        /**
         * @brief 대상 객체를 소유하지 않고 상대 참조의 핸들과 GUID를 복사하거나 이동합니다.
         * @tparam U 변환 또는 비교할 참조 대상의 형식입니다.
         * @param[in] Other 복사·이동 또는 비교할 상대 참조입니다.
         */
        template <typename U> requires std::is_convertible_v<U*, T*>
        TObjectRef(const CoreUObject::TObjectRef<U>& Other);

    public:
        /**
         * @brief 세대 핸들과 참조 대상 형식으로 살아 있는 객체를 조회합니다.
         * @return 대상 객체의 비소유 주소이며 수명이 끝났거나 형식이 맞지 않으면 nullptr입니다.
         */
        T* Get() const;

        /**
         * @brief 참조가 현재 살아 있는 호환 객체로 해석되는지 검사합니다.
         * @return 객체를 해석할 수 있으면 true입니다.
         */
        bool IsValid() const;

        /**
         * @brief 객체의 수명에 영향을 주지 않고 핸들과 GUID를 빈 참조로 초기화합니다.
         */
        void Reset();

        /**
         * @brief 보관한 런타임 세대 핸들을 반환합니다.
         * @return 레지스트리 슬롯과 세대로 구성된 핸들입니다.
         */
        CoreUObject::FObjectHandle GetHandle() const;

        /**
         * @brief 생성 시 기록한 객체의 영속 식별자를 반환합니다.
         * @return 보관한 GUID이며 빈 참조이면 널 GUID입니다.
         */
        const Core::FGuid& GetGuid() const;

        /**
         * @brief 런타임 핸들과 레지스트리에서 조회한 GUID로 객체 참조를 만듭니다.
         * @param[in] Handle 해석하거나 해시할 런타임 객체 세대 핸들입니다.
         * @return 핸들을 보관하는 비소유 참조입니다.
         */
        static CoreUObject::TObjectRef<T> FromHandle(CoreUObject::FObjectHandle Handle);

        /**
         * @brief 살아 있는 참조 대상을 포인터로 조회합니다.
         * @return Get()과 같은 비소유 주소입니다.
         */
        T* operator->() const;

        /**
         * @brief 참조 대상을 역참조합니다. 호출 전에 참조가 유효해야 합니다.
         * @return 대상 객체의 참조입니다.
         */
        T& operator*() const;

        /**
         * @brief 참조를 대상 객체의 비소유 포인터로 변환합니다.
         * @return Get()과 같은 주소이며 해석할 수 없으면 nullptr입니다.
         */
        operator T*() const;

        /**
         * @brief 참조 대상의 현재 생존 여부를 검사합니다.
         * @return 참조가 살아 있는 객체로 해석되면 true입니다.
         */
        explicit operator bool() const;

        /**
         * @brief 참조 대상을 해석할 수 없는지 검사합니다.
         * @return Get()이 nullptr이면 true이며 이미 파괴된 대상도 포함합니다.
         */
        bool operator==(std::nullptr_t) const;

        /**
         * @brief 해석한 대상 포인터를 전달받은 객체 주소와 비교합니다.
         * @tparam U 변환 또는 비교할 참조 대상의 형식입니다.
         * @param[in] Object 비소유 참조를 만들거나 비교할 객체입니다.
         * @return Get()과 Object의 주소가 같으면 true입니다.
         */
        template <typename U> requires (std::is_convertible_v<U*, const T*> || std::is_convertible_v<T*, const U*>)
        bool operator==(U* Object) const;

        /**
         * @brief 핸들을 비교하며 핸들 표현이 무효한 경우에는 GUID도 함께 비교합니다.
         * @tparam U 변환 또는 비교할 참조 대상의 형식입니다.
         * @param[in] Other 복사·이동 또는 비교할 상대 참조입니다.
         * @return 핸들이 같고 유효한 표현이거나, 무효 핸들과 GUID가 모두 같으면 true입니다.
         */
        template <typename U> requires (std::is_convertible_v<const U*, const T*> || std::is_convertible_v<const T*, const U*>)
        bool operator==(const CoreUObject::TObjectRef<U>& Other) const;

    private:
        /**
         * @brief TObjectRef에서 내부 연결 또는 저장 상태에 접근하도록 허용합니다.
         * @tparam U 변환 또는 비교할 참조 대상의 형식입니다.         */
        template <typename U>
        friend class TObjectRef;

        /**
         * @brief FObjectReference에서 내부 연결 또는 저장 상태에 접근하도록 허용합니다.
         */
        friend struct FObjectReference;

    private:
        /**
         * @brief 객체 수명과 슬롯 재사용을 판별하는 세대 핸들입니다.
         */
        CoreUObject::FObjectHandle mHandle{};

        /**
         * @brief 객체 참조의 영속 식별자입니다.
         */
        Core::FGuid mGuid{Core::FGuid::Null()};
    };

    /**
     * @brief 객체 참조의 런타임 핸들을 해시하는 함수 객체입니다.
     * @tparam T 참조 대상의 형식입니다.
     */
    template <typename T>
    struct TObjectRefHash {
    public:
        /**
         * @brief 객체 참조 또는 핸들의 해시 값을 계산합니다.
         * @param[in] Reference 변환하거나 해시할 객체 참조입니다.
         * @return 런타임 핸들에 대응하는 해시 값입니다.
         */
        std::size_t operator()(const CoreUObject::TObjectRef<T>& Reference) const;
    };

    namespace ObjectHandlePrivate {

        template <typename T>
        struct TObjectHashType<CoreUObject::TObjectRef<T>> {
        public:
            using Type = CoreUObject::TObjectRefHash<T>;
        };

    }

    /**
     * @brief 형식이 TObjectRef 특수화인지 판별하고 참조 대상 형식을 제공합니다.
     * @tparam T 참조 대상의 형식입니다.
     */
    template <typename T>
    struct TIsObjectRef : std::false_type {
    };

    /**
     * @brief TObjectRef 형식을 참으로 판별하고 참조 대상 형식을 노출하는 특수화입니다.
     * @tparam T 참조 대상의 형식입니다.
     */
    template <typename T>
    struct TIsObjectRef<CoreUObject::TObjectRef<T>> : std::true_type {
    public:
        /**
         * @brief 객체 참조가 가리키는 대상 형식입니다.
         */
        using FObjectType = T;
    };

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(T* Object);

    template <typename T, typename TDeleter>
    CoreUObject::TObjectRef<T> MakeObjectRef(const std::unique_ptr<T, TDeleter>& Object);

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(const std::shared_ptr<T>& Object);

    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(const CoreUObject::TObjectRef<T>& Object);


    template <typename T>
    TObjectRef<T>::TObjectRef(std::nullptr_t) {
    }

    template <typename T>
    TObjectRef<T>::TObjectRef(T* Object) {
        if (Object != nullptr) {
            mHandle = Object->GetHandle();
            mGuid = Object->GetGuid();
        }
    }

    template <typename T>
    template <typename U> requires std::is_convertible_v<U*, T*>
    TObjectRef<T>::TObjectRef(const CoreUObject::TObjectRef<U>& Other)
		: mHandle{Other.mHandle},
		  mGuid{Other.mGuid} {
    }

    template <typename T>
    T* TObjectRef<T>::Get() const {
        using FMutableType = std::remove_const_t<T>;
        static_assert(std::is_base_of_v<CoreUObject::UObject, FMutableType>);
        static_assert(std::is_same_v<typename FMutableType::ReflectionOwner, FMutableType>);

        return static_cast<T*>(CoreUObject::ObjectRefPrivate::Resolve(mHandle, FMutableType::StaticClass()));
    }

    template <typename T>
    bool TObjectRef<T>::IsValid() const {
        return Get() != nullptr;
    }

    template <typename T>
    void TObjectRef<T>::Reset() {
        mHandle = {};
        mGuid = Core::FGuid::Null();
    }

    template <typename T>
    CoreUObject::FObjectHandle TObjectRef<T>::GetHandle() const {
        return mHandle;
    }

    template <typename T>
    const Core::FGuid& TObjectRef<T>::GetGuid() const {
        return mGuid;
    }

    template <typename T>
    CoreUObject::TObjectRef<T> TObjectRef<T>::FromHandle(CoreUObject::FObjectHandle Handle) {
        CoreUObject::TObjectRef<T> Reference{};
        Reference.mHandle = Handle;
        Reference.mGuid = CoreUObject::ObjectRefPrivate::GetGuid(Handle);

        return Reference;
    }

    template <typename T>
    T* TObjectRef<T>::operator->() const {
        return Get();
    }

    template <typename T>
    T& TObjectRef<T>::operator*() const {
        return *Get();
    }

    template <typename T>
    TObjectRef<T>::operator T*() const {
        return Get();
    }

    template <typename T>
    TObjectRef<T>::operator bool() const {
        return IsValid();
    }

    template <typename T>
    bool TObjectRef<T>::operator==(std::nullptr_t) const {
        return Get() == nullptr;
    }

    template <typename T>
    template <typename U> requires (std::is_convertible_v<U*, const T*> || std::is_convertible_v<T*, const U*>)
    bool TObjectRef<T>::operator==(U* Object) const {
        return Get() == Object;
    }

    template <typename T>
    template <typename U> requires (std::is_convertible_v<const U*, const T*> || std::is_convertible_v<const T*, const U*>)
    bool TObjectRef<T>::operator==(const CoreUObject::TObjectRef<U>& Other) const {
        return mHandle == Other.mHandle && (mHandle.IsValid() || mGuid == Other.mGuid);
    }

    template <typename T>
    std::size_t TObjectRefHash<T>::operator()(const CoreUObject::TObjectRef<T>& Reference) const {
        return CoreUObject::FObjectHandleHash{}(Reference.GetHandle());
    }

    /**
     * @brief 객체 소유권을 이전하지 않고 형식 지정 비소유 참조를 만듭니다.
     * @tparam T 참조 대상의 형식입니다.
     * @param[in] Object 비소유 참조를 만들거나 비교할 객체입니다.
     * @return 객체의 핸들과 GUID를 보관하는 참조입니다.
     */
    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(T* Object) {
        return CoreUObject::TObjectRef<T>{Object};
    }

    /**
     * @brief 객체 소유권을 이전하지 않고 형식 지정 비소유 참조를 만듭니다.
     * @tparam T 참조 대상의 형식입니다.
     * @tparam TDeleter 소유 포인터의 삭제자 형식입니다.
     * @param[in] Object 비소유 참조를 만들거나 비교할 객체입니다.
     * @return 객체의 핸들과 GUID를 보관하는 참조입니다.
     */
    template <typename T, typename TDeleter>
    CoreUObject::TObjectRef<T> MakeObjectRef(const std::unique_ptr<T, TDeleter>& Object) {
        return CoreUObject::TObjectRef<T>{Object.get()};
    }

    /**
     * @brief 객체 소유권을 이전하지 않고 형식 지정 비소유 참조를 만듭니다.
     * @tparam T 참조 대상의 형식입니다.
     * @param[in] Object 비소유 참조를 만들거나 비교할 객체입니다.
     * @return 객체의 핸들과 GUID를 보관하는 참조입니다.
     */
    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(const std::shared_ptr<T>& Object) {
        return CoreUObject::TObjectRef<T>{Object.get()};
    }

    /**
     * @brief 객체 소유권을 이전하지 않고 형식 지정 비소유 참조를 만듭니다.
     * @tparam T 참조 대상의 형식입니다.
     * @param[in] Object 비소유 참조를 만들거나 비교할 객체입니다.
     * @return 객체의 핸들과 GUID를 보관하는 참조입니다.
     */
    template <typename T>
    CoreUObject::TObjectRef<T> MakeObjectRef(const CoreUObject::TObjectRef<T>& Object) {
        return Object;
    }

}
