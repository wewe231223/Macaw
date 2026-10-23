/**
 * @file TEngineAllocator.h
 * @brief 엔진 메모리 통계와 정렬된 할당을 사용하는 표준 컨테이너용 할당자를 정의한다.
 */
#pragma once
#include "Core/Base/ErrorHandler.h"
#include "Core/Memory/Memory.h"

#include <cstddef>
#include <type_traits>
#include <limits>
#include <new>

/**
 * @brief Memory의 할당 및 해제 기능을 표준 컨테이너의 할당자 인터페이스로 제공한다.
 * @tparam T 할당할 요소 타입.
 * @tparam Tag 각 할당에 적용할 메모리 통계 태그. 기본값은 Container이다.
 * @details 요소를 생성하거나 소멸시키지 않고 저장 공간만 관리한다. 같은 태그의 할당자는 요소 타입에 관계없이 서로 교환하여 메모리를 해제할 수 있다.
 */
template <typename T, Memory::EMemoryTag Tag = Memory::EMemoryTag::Container>
class TEngineAllocator {
public:
    /**
     * @brief 표준 할당자 인터페이스가 요구하는 요소 타입.
     */
    using value_type = T;
    /**
     * @brief 요소 개수와 저장 공간 크기에 사용하는 부호 없는 타입.
     */
    using size_type = std::size_t;
    /**
     * @brief 요소 간 거리 표현에 사용하는 부호 있는 타입.
     */
    using difference_type = std::ptrdiff_t;

    /**
     * @brief 같은 할당자 타입의 인스턴스가 항상 동등함을 나타내는 표준 특성.
     */
    using is_always_equal = std::true_type;

    /**
     * @brief 인스턴스별 상태가 없는 엔진 할당자를 생성한다.
     */
    constexpr TEngineAllocator() noexcept = default;

    /**
     * @brief 같은 통계 태그의 다른 요소 타입 할당자에서 변환 생성한다.
     * @tparam U 원본 할당자의 요소 타입.
     * @details 인스턴스별 상태가 없으므로 추가 작업을 수행하지 않는다.
     */
    template <typename U>
    constexpr TEngineAllocator(const TEngineAllocator<U, Tag>&) noexcept;

    /**
     * @brief 지정한 요소 개수의 저장 공간을 할당한다.
     * @param Count 저장할 요소 수.
     * @return Count * sizeof(T) 바이트를 저장할 수 있고 alignof(T)를 만족하는 포인터.
     * @details Count가 max_size()를 넘으면 치명적 오류를 보고한다. 생성된 저장 공간의 태그는 Tag이며 요소 생성은 호출자가 수행한다.
     */
    T* allocate(size_type Count);

    /**
     * @brief 할당한 요소 저장 공간을 해제한다.
     * @param Ptr 같은 태그의 엔진 할당자가 반환한 저장 공간. nullptr도 허용한다.
     * @details 두 번째 요소 개수 인수는 사용하지 않는다. 실제 해제 크기와 태그는 Memory의 내부 할당 헤더에서 확인한다.
     */
    void deallocate(T* Ptr, size_type) noexcept;

    /**
     * @brief 바이트 크기 계산이 넘치지 않는 최대 요소 개수를 반환한다.
     * @return size_type의 최댓값을 sizeof(T)로 나눈 값.
     */
    constexpr size_type max_size() const noexcept;

    /**
     * @brief 통계 태그를 유지하면서 다른 요소 타입의 할당자를 지정한다.
     * @tparam U 새 요소 타입.
     */
    template <typename U>
    struct rebind {
        /**
         * @brief 요소 타입만 U로 바꾸고 태그를 유지한 할당자 타입.
         */
        using other = TEngineAllocator<U, Tag>;
    };
};

/**
 * @brief 같은 태그의 엔진 할당자를 동등한 것으로 비교한다.
 * @tparam T 첫 번째 할당자의 요소 타입.
 * @tparam U 두 번째 할당자의 요소 타입.
 * @tparam Tag 두 할당자가 공유하는 메모리 태그.
 * @return 인스턴스별 상태가 없으므로 항상 true.
 */
template <typename T, typename U, Memory::EMemoryTag Tag>
constexpr bool operator==(const TEngineAllocator<T, Tag>&, const TEngineAllocator<U, Tag>&) noexcept {
    return true;
}

/**
 * @brief 같은 태그의 엔진 할당자가 다른지 비교한다.
 * @tparam T 첫 번째 할당자의 요소 타입.
 * @tparam U 두 번째 할당자의 요소 타입.
 * @tparam Tag 두 할당자가 공유하는 메모리 태그.
 * @return 항상 false.
 */
template <typename T, typename U, Memory::EMemoryTag Tag>
constexpr bool operator!=(const TEngineAllocator<T, Tag>&, const TEngineAllocator<U, Tag>&) noexcept {
    return false;
}

template <typename T, Memory::EMemoryTag Tag>
template <typename U>
constexpr TEngineAllocator<T, Tag>::TEngineAllocator(const TEngineAllocator<U, Tag>&) noexcept {
}

template <typename T, Memory::EMemoryTag Tag>
T* TEngineAllocator<T, Tag>::allocate(size_type Count) {
    if (Count > max_size()) {
        ErrorHandler::Report("TEngineAllocator::allocate", "The requested element count exceeds the allocator capacity.", ErrorHandler::EErrorLevel::Critical);
    }

    return static_cast<T*>(Memory::Allocate(Count * sizeof(T), alignof(T), Tag));
}

template <typename T, Memory::EMemoryTag Tag>
void TEngineAllocator<T, Tag>::deallocate(T* Ptr, size_type) noexcept {
    Memory::Free(Ptr);
}

template <typename T, Memory::EMemoryTag Tag>
constexpr typename TEngineAllocator<T, Tag>::size_type TEngineAllocator<T, Tag>::max_size() const noexcept {
    return (std::numeric_limits<size_type>::max)() / sizeof(T);
}
