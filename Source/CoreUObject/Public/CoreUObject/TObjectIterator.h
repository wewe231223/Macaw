/**
 * @file TObjectIterator.h
 * @brief 타입과 Outer로 필터링하는 객체 스냅샷 반복자입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/Reflection/UClass.h"

#include <concepts>
#include <iterator>
#include <type_traits>

namespace CoreUObject {

    /**
     * @brief 타입별 객체 범위를 선언합니다.
     * @tparam T UObject 파생 타입입니다.
     */
    template <typename T>
    class TObjectRange;

    /**
     * @brief 생성 시 존재한 타입 호환 객체를 순회하고 소멸한 객체를 건너뜁니다.
     * @tparam T UObject 파생 타입이며 const 타입도 지원합니다.
     * @note 객체 수명을 소유하지 않습니다. 생성 이후 추가된 객체와 재사용된 슬롯은 제외합니다.
     */
    template <typename T>
    class TObjectIterator {
    public:
        /**
         * @brief 리플렉션 소유 타입의 별칭입니다.
         */
        ReflectTypes(TObjectIterator);
        static_assert(std::derived_from<std::remove_const_t<T>, CoreUObject::UObject>);

    public:
        /**
         * @brief 타입 및 Outer 필터의 스냅샷을 만듭니다.
         * @param Outer 하위 객체의 기준 Outer입니다.
         * @param IncludeDerived 파생 타입의 포함 여부입니다.
         */
        explicit TObjectIterator(const CoreUObject::UObject* Outer = nullptr, bool IncludeDerived = true);

    private:
        /**
         * @brief 기존 스냅샷에서 반복자를 만듭니다.
         * @param Handles 공유할 객체 핸들 스냅샷입니다.
         */
        explicit TObjectIterator(std::shared_ptr<const Core::TArray<CoreUObject::FObjectHandle>> Handles);

    public:
        /**
         * @brief 정적 타입 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터입니다.
         */
        ReflectTemplate(TObjectIterator);

        /**
         * @brief 현재 객체를 반환합니다.
         * @return 현재 객체이며, 끝에 도달하면 nullptr입니다.
         */
        T* Get() const;

        /**
         * @brief 현재 객체를 반환합니다.
         * @return 현재 객체이며, 끝에 도달하면 nullptr입니다.
         */
        T* operator*() const;

        /**
         * @brief 현재 객체의 멤버에 접근합니다.
         * @return 유효한 객체 주소이며 끝에서는 nullptr입니다.
         */
        T* operator->() const;

        /**
         * @brief 남은 유효 객체를 검사합니다.
         * @return 순회할 객체가 있으면 true입니다.
         */
        explicit operator bool() const;

        /**
         * @brief 다음 살아 있는 객체로 이동합니다.
         * @return 이 반복자의 참조입니다.
         */
        CoreUObject::TObjectIterator<T>& operator++();

        /**
         * @brief 이동 전 반복자를 보관하고 전진합니다.
         * @return 이동 전 반복자의 사본입니다.
         */
        CoreUObject::TObjectIterator<T> operator++(int);

        /**
         * @brief 끝 센티널과 비교합니다.
         * @param Sentinel 범위의 끝입니다.
         * @return 유효한 객체가 없으면 true입니다.
         */
        bool operator==(std::default_sentinel_t Sentinel) const;

        /**
         * @brief 반복자의 위치를 비교합니다.
         * @param Other 비교할 반복자입니다.
         * @return 같은 스냅샷의 같은 유효 위치이면 true입니다.
         */
        bool operator==(const CoreUObject::TObjectIterator<T>& Other) const;

    private:
        /**
         * @brief 생성된 리플렉션 멤버 등록 함수의 접근을 허용합니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 범위가 같은 핸들 스냅샷을 공유하도록 허용합니다.
         */
        friend class TObjectRange<T>;

    private:
        /**
         * @brief 순회 대상의 세대 핸들 스냅샷입니다.
         */
        std::shared_ptr<const Core::TArray<CoreUObject::FObjectHandle>> mHandles{};
        ReflectProperty(mHandles, "Handles", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 소멸한 객체를 건너뛴 현재 위치입니다.
         */
        mutable std::size_t mIndex{};
        ReflectProperty(mIndex, "Index", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief range-for에서 타입별 객체를 순회하는 범위입니다.
     * @tparam T UObject 파생 타입입니다.
     */
    template <typename T>
    class TObjectRange {
    public:
        /**
         * @brief 리플렉션 소유 타입의 별칭입니다.
         */
        ReflectTypes(TObjectRange);
        static_assert(std::derived_from<std::remove_const_t<T>, CoreUObject::UObject>);

    public:
        /**
         * @brief 필터에 일치하는 객체를 수집합니다.
         * @param Outer 하위 객체의 기준 Outer입니다.
         * @param IncludeDerived 파생 타입의 포함 여부입니다.
         */
        explicit TObjectRange(const CoreUObject::UObject* Outer = nullptr, bool IncludeDerived = true);

    public:
        /**
         * @brief 정적 타입 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터입니다.
         */
        ReflectTemplate(TObjectRange);

        /**
         * @brief 범위 시작을 반환합니다.
         * @return 스냅샷을 공유하는 반복자입니다.
         */
        CoreUObject::TObjectIterator<T> begin() const;

        /**
         * @brief 범위 끝을 반환합니다.
         * @return 객체가 남지 않았음을 나타내는 센티널입니다.
         */
        std::default_sentinel_t end() const;

    private:
        /**
         * @brief 생성된 리플렉션 멤버 등록 함수의 접근을 허용합니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 시작 반복자들 사이에 공유하는 핸들 스냅샷입니다.
         */
        std::shared_ptr<const Core::TArray<CoreUObject::FObjectHandle>> mHandles{};
        ReflectProperty(mHandles, "Handles", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    template <typename T>
    CoreUObject::TObjectIterator<T>::TObjectIterator(const CoreUObject::UObject* Outer, bool IncludeDerived)
		: mHandles{std::make_shared<const Core::TArray<CoreUObject::FObjectHandle>>(CoreUObject::FObjectRegistry::Get()->GetObjects(T::StaticClass(), Outer, IncludeDerived))} {
    }

    template <typename T>
    CoreUObject::TObjectIterator<T>::TObjectIterator(std::shared_ptr<const Core::TArray<CoreUObject::FObjectHandle>> Handles)
		: mHandles{std::move(Handles)} {
    }

    template <typename T>
    T* CoreUObject::TObjectIterator<T>::Get() const {
        while (mHandles != nullptr && mIndex < mHandles->size()) {
            CoreUObject::UObject* Object{CoreUObject::FObjectRegistry::Get()->Resolve((*mHandles)[mIndex])};

            if (Object != nullptr && Object->IsA(T::StaticClass())) {
                return static_cast<T*>(Object);
            }

            ++mIndex;
        }

        return nullptr;
    }

    template <typename T>
    T* CoreUObject::TObjectIterator<T>::operator*() const {
        return Get();
    }

    template <typename T>
    T* CoreUObject::TObjectIterator<T>::operator->() const {
        return Get();
    }

    template <typename T>
    CoreUObject::TObjectIterator<T>::operator bool() const {
        return Get() != nullptr;
    }

    template <typename T>
    CoreUObject::TObjectIterator<T>& CoreUObject::TObjectIterator<T>::operator++() {
        if (mHandles != nullptr && mIndex < mHandles->size()) {
            ++mIndex;
        }

        Get();

        return *this;
    }

    template <typename T>
    CoreUObject::TObjectIterator<T> CoreUObject::TObjectIterator<T>::operator++(int) {
        CoreUObject::TObjectIterator<T> Previous{*this};

        ++*this;
        return Previous;
    }

    template <typename T>
    bool CoreUObject::TObjectIterator<T>::operator==(std::default_sentinel_t) const {
        return Get() == nullptr;
    }

    template <typename T>
    bool CoreUObject::TObjectIterator<T>::operator==(const CoreUObject::TObjectIterator<T>& Other) const {
        Get();
        Other.Get();
        return mHandles == Other.mHandles && mIndex == Other.mIndex;
    }

    template <typename T>
    CoreUObject::TObjectRange<T>::TObjectRange(const CoreUObject::UObject* Outer, bool IncludeDerived)
		: mHandles{std::make_shared<const Core::TArray<CoreUObject::FObjectHandle>>(CoreUObject::FObjectRegistry::Get()->GetObjects(T::StaticClass(), Outer, IncludeDerived))} {
    }

    template <typename T>
    CoreUObject::TObjectIterator<T> CoreUObject::TObjectRange<T>::begin() const {
        return CoreUObject::TObjectIterator<T>{mHandles};
    }

    template <typename T>
    std::default_sentinel_t CoreUObject::TObjectRange<T>::end() const {
        return {};
    }

}

#include "TObjectIteratorReflection.generated.h"

#include "TObjectRangeReflection.generated.h"
