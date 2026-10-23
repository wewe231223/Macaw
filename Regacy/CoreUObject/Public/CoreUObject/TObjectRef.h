/**
 * @file TObjectRef.h
 * @brief 등록 핸들로 객체를 찾고 리플렉션 타입을 검증하는 비소유 참조를 제공합니다.
 */
#pragma once
#include "CoreUObject/Reflection/UClass.h"

#include "Core/CoreMinimal.h"

#include <cstddef>
#include <type_traits>
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/UObjectSystem.h"

/**
 * @brief 객체 수명을 연장하지 않는 타입 검사형 핸들 참조입니다.
 * @tparam T 자체 리플렉션 정보를 선언한 UObject 파생 타입입니다.
 * @note 객체가 등록 해제되거나 슬롯 세대가 바뀌면 Get은 nullptr를 반환합니다.
 */
template <typename T>
class TObjectRef {
public:
    /**
     * @brief 무효 핸들을 갖는 빈 참조로 초기화합니다.
     */
    TObjectRef() = default;

    /**
     * @brief nullptr를 받아 빈 참조로 초기화합니다.
     */
    TObjectRef(std::nullptr_t) noexcept;

    /**
     * @brief 객체의 현재 등록 핸들을 저장합니다.
     * @param Object 참조할 객체입니다. nullptr이면 빈 참조입니다.
     */
    TObjectRef(T* Object) noexcept;

    /**
     * @brief 전달한 핸들을 타입 검사 없이 저장합니다.
     * @param InHandle 저장할 등록 핸들입니다.
     */
    explicit TObjectRef(FObjectHandle InHandle) noexcept;

    /**
     * @brief 현재 핸들을 해석하고 T와의 상속 관계를 검사합니다.
     * @return 살아 있는 T 호환 객체입니다. 핸들 해석 또는 타입 검사에 실패하면 nullptr입니다.
     */
    T* Get() const noexcept;

    /**
     * @brief Get으로 해석한 객체 포인터를 반환합니다.
     * @pre 멤버 접근 시 참조가 유효해야 합니다.
     * @return Get의 결과입니다.
     */
    T* operator->() const noexcept;

    /**
     * @brief Get으로 해석한 객체의 참조를 반환합니다.
     * @pre 참조가 유효해야 합니다.
     * @return 참조 대상 객체입니다.
     */
    T& operator*() const noexcept;

    /**
     * @brief 타입 검사를 포함한 현재 참조의 유효성을 반환합니다.
     * @return IsValid의 결과입니다.
     */
    explicit operator bool() const noexcept;

    /**
     * @brief 현재 핸들이 T 호환 객체로 해석되는지 검사합니다.
     * @return Get이 nullptr가 아니면 true입니다.
     */
    bool IsValid() const noexcept;

    /**
     * @brief 객체의 현재 등록 핸들로 참조를 교체합니다.
     * @param Object 참조할 객체입니다. nullptr이면 무효 핸들을 저장합니다.
     */
    void Set(T* Object) noexcept;

    /**
     * @brief 참조 핸들을 즉시 교체합니다.
     * @param InHandle 새 핸들입니다.
     * @note 타입과 생존 여부는 Get 호출 시 검사합니다.
     */
    void SetHandle(FObjectHandle InHandle) noexcept;

    /**
     * @brief 핸들을 비워 참조를 해제합니다.
     */
    void Reset() noexcept;

    /**
     * @brief 저장된 핸들을 반환합니다.
     * @return 타입 또는 생존 여부를 추가로 검사하지 않은 핸들입니다.
     */
    FObjectHandle GetHandle() const noexcept;

private:
    /**
     * @brief T의 UObject 상속과 자체 리플렉션 선언을 컴파일 시 검사합니다.
     */
    static constexpr void ValidateType() noexcept;

    /**
     * @brief 대상 객체의 슬롯과 세대를 저장한 비소유 핸들입니다.
     */
    FObjectHandle mHandle{};
};

template <typename T>
TObjectRef<T>::TObjectRef(std::nullptr_t) noexcept {
}

template <typename T>
TObjectRef<T>::TObjectRef(T* Object) noexcept {
    Set(Object);
}

template <typename T>
TObjectRef<T>::TObjectRef(FObjectHandle InHandle) noexcept
	: mHandle(InHandle) {
}

template <typename T>
T* TObjectRef<T>::Get() const noexcept {
    ValidateType();

    UObject* Object{UObjectSystem::Resolve(mHandle)};

    if (Object == nullptr || !Object->GetClass()->IsChildOf(T::StaticClass())) {
        return nullptr;
    }

    return static_cast<T*>(Object);
}

template <typename T>
T* TObjectRef<T>::operator->() const noexcept {
    return Get();
}

template <typename T>
T& TObjectRef<T>::operator*() const noexcept {
    return *Get();
}

template <typename T>
TObjectRef<T>::operator bool() const noexcept {
    return IsValid();
}

template <typename T>
bool TObjectRef<T>::IsValid() const noexcept {
    return Get() != nullptr;
}

template <typename T>
void TObjectRef<T>::Set(T* Object) noexcept {
    ValidateType();

    mHandle = Object != nullptr ? Object->GetHandle() : FObjectHandle{};
}

template <typename T>
void TObjectRef<T>::SetHandle(FObjectHandle InHandle) noexcept {
    mHandle = InHandle;
}

template <typename T>
void TObjectRef<T>::Reset() noexcept {
    mHandle = {};
}

template <typename T>
FObjectHandle TObjectRef<T>::GetHandle() const noexcept {
    return mHandle;
}

template <typename T>
constexpr void TObjectRef<T>::ValidateType() noexcept {
    static_assert(std::is_base_of_v<UObject, T>, "TObjectRef<T> requires T to derive from UObject.");
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>, "TObjectRef<T> requires T to declare its own type information.");
}
