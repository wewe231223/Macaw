/**
 * @file FMessage.h
 * @brief 메시지 타입 제약과 인라인 또는 힙 저장소를 사용하는 이동 전용 메시지 컨테이너를 정의한다.
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include "Core/Channel/FMessageTypeInfo.h"
#include "Core/Memory/Memory.h"
/**
 * @brief 정적 타입 정보와 예외 없는 이동 생성 및 소멸을 제공하는 메시지 타입의 제약.
 * @tparam T 검사할 메시지 타입.
 * @details StaticTypeInfo()는 const FMessageTypeInfo 참조를 반환해야 한다. 타입 정보의 주소는 해당 타입을 식별하는 데 사용한다.
 */
template <typename T>
concept CMessageType = requires {
    {
        T::StaticTypeInfo()
    } -> std::same_as<const FMessageTypeInfo&>;
} && std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>;

/**
 * @brief 지정한 인수로 예외 없이 생성할 수 있는 메시지 타입의 제약.
 * @tparam T CMessageType을 만족해야 하는 메시지 타입.
 * @tparam Args 메시지 생성자에 전달할 인수 타입들.
 */
template <typename T, typename... Args>
concept CMessageConstructible = CMessageType<T> && std::is_nothrow_constructible_v<T, Args...>;

/**
 * @brief 구체적인 타입을 지운 상태로 메시지 하나의 소유권과 수명을 관리한다.
 * @details 크기와 정렬 요구가 내부 버퍼에 맞으면 인라인으로 저장하고, 그 외에는 Memory의 메시지 태그로 힙을 할당한다. 복사는 금지하며 이동 시 원본을 비운다.
 * @note 저장된 메시지의 타입 정보 객체는 메시지를 사용하는 동안 유효하며 같은 주소를 유지해야 한다.
 */
class FMessage {
public:
    /**
     * @brief 인라인 저장소의 크기. 단위는 바이트이며 기본 크기는 64이다.
     */
    static constexpr std::size_t InlineSize{64};

    /**
     * @brief 인라인 저장소가 제공하는 정렬 크기.
     */
    static constexpr std::size_t InlineAlignment{alignof(std::max_align_t)};

private:
    /**
     * @brief 저장된 페이로드를 소멸시키고 필요하면 힙을 해제하는 예외 없는 콜백 타입.
     */
    using FDestroyFunction = void (*)(FMessage&) noexcept;

    /**
     * @brief 목적지와 원본 메시지를 받아 인라인 페이로드를 이동하는 예외 없는 콜백 타입.
     */
    using FMoveFunction = void (*)(FMessage&, FMessage&) noexcept;

public:
    /**
     * @brief 타입 정보와 페이로드가 없는 빈 메시지를 생성한다.
     */
    FMessage() noexcept = default;

    /**
     * @brief 메시지 소유권의 복사 생성을 금지한다.
     */
    FMessage(const FMessage&) = delete;

    /**
     * @brief 메시지 소유권의 복사 할당을 금지한다.
     */
    FMessage& operator=(const FMessage&) = delete;

    /**
     * @brief 원본의 메시지를 이동하여 새 컨테이너를 생성한다.
     * @param Other 이동할 메시지. 이동 후 빈 상태가 된다.
     */
    FMessage(FMessage&& Other) noexcept;

    /**
     * @brief 기존 메시지를 정리한 뒤 원본의 메시지를 이동한다.
     * @param Other 이동할 메시지. 자기 자신이 아니면 이동 후 빈 상태가 된다.
     * @return 현재 객체의 참조.
     * @details 자기 이동 할당은 상태를 변경하지 않는다.
     */
    FMessage& operator=(FMessage&& Other) noexcept;

    /**
     * @brief 저장된 메시지를 소멸시키고 메시지용 힙 할당을 해제한다.
     */
    ~FMessage() noexcept;

public:
    /**
     * @brief 기존 메시지를 정리하고 지정한 타입의 메시지를 새로 생성한다.
     * @tparam T 전달한 인수로 예외 없이 생성 가능한 메시지 타입.
     * @tparam Args 생성자 인수의 타입들.
     * @param Arguments 메시지 생성자에 완벽 전달할 인수들.
     * @details 크기와 정렬 조건에 따라 인라인 또는 힙 저장소를 선택한다. 힙 할당 실패는 Memory의 치명적 오류 처리에 따른다.
     */
    template <typename T, typename... Args>
        requires CMessageConstructible<T, Args...>
    void TryEmplace(Args&&... Arguments);

    /**
     * @brief 저장된 메시지 페이로드가 있는지 확인한다.
     * @return 페이로드 포인터가 있으면 true, 빈 메시지이면 false.
     */
    [[nodiscard]] bool IsValid() const noexcept;

    /**
     * @brief 저장된 메시지의 타입 정보를 조회한다.
     * @return 해당 메시지 타입의 정적 타입 정보 포인터.
     * @pre 유효한 메시지에서 호출해야 한다.
     * @details 타입 정보가 없으면 치명적 오류를 보고하고 프로세스를 종료한다.
     */
    [[nodiscard]] const FMessageTypeInfo* GetTypeInfo() const noexcept;

    /**
     * @brief 저장된 메시지가 정확히 지정한 타입인지 확인한다.
     * @tparam T 확인할 메시지 타입.
     * @return 저장된 타입 정보의 주소가 T의 정적 타입 정보 주소와 같으면 true. 빈 메시지는 false.
     */
    template <CMessageType T>
    [[nodiscard]] bool Is() const noexcept;

    /**
     * @brief 지정한 타입으로 메시지 페이로드를 조회한다.
     * @tparam T 조회할 메시지 타입.
     * @return 타입이 일치하면 컨테이너가 소유하는 페이로드 포인터, 그 외에는 nullptr.
     * @note 메시지의 재설정, 교체 또는 이동으로 반환한 포인터가 무효화될 수 있다.
     */
    template <CMessageType T>
    [[nodiscard]] T* Get() noexcept;

    /**
     * @brief 지정한 타입의 메시지 페이로드를 읽기 전용으로 조회한다.
     * @tparam T 조회할 메시지 타입.
     * @return 타입이 일치하면 컨테이너가 소유하는 페이로드의 읽기 전용 포인터, 그 외에는 nullptr.
     * @note 메시지의 재설정, 교체 또는 이동으로 반환한 포인터가 무효화될 수 있다.
     */
    template <CMessageType T>
    [[nodiscard]] const T* Get() const noexcept;

    /**
     * @brief 저장된 페이로드와 할당을 정리하고 메시지를 빈 상태로 만든다.
     * @details 이미 비어 있어도 호출할 수 있다. 페이로드가 있는데 소멸 콜백이 없으면 치명적 오류를 보고한다.
     */
    void Reset() noexcept;

private:
    /**
     * @brief 타입의 크기와 정렬 요구가 인라인 저장소에 맞는지 확인한다.
     * @tparam T 검사할 페이로드 타입.
     * @return 크기가 InlineSize 이하이고 정렬이 InlineAlignment 이하이면 true.
     */
    template <typename T>
    static constexpr bool CanStoreInline() noexcept;

    /**
     * @brief 인라인 저장소의 페이로드를 소멸시킨다.
     * @tparam T 저장된 페이로드 타입.
     * @param Message T 타입의 인라인 페이로드를 가진 메시지.
     */
    template <typename T>
    static void DestroyInline(FMessage& Message) noexcept;

    /**
     * @brief 힙 페이로드를 소멸시키고 메모리를 해제한다.
     * @tparam T 저장된 페이로드 타입.
     * @param Message T 타입의 힙 페이로드를 가진 메시지.
     */
    template <typename T>
    static void DestroyHeap(FMessage& Message) noexcept;

    /**
     * @brief 인라인 페이로드를 목적지의 내부 저장소로 이동한다.
     * @tparam T 이동할 페이로드 타입.
     * @param Destination 새 페이로드를 생성할 메시지.
     * @param Source T 타입의 인라인 페이로드를 가진 원본. 원본 페이로드를 소멸시키고 데이터 포인터를 비운다.
     * @details 타입 정보와 나머지 메타데이터의 이동은 MoveFrom()에서 처리한다.
     */
    template <typename T>
    static void MoveInline(FMessage& Destination, FMessage& Source) noexcept;

    /**
     * @brief 타입 정보와 페이로드 소유권을 원본에서 가져온다.
     * @param Other 이동할 원본 메시지. 유효한 메시지를 이동하면 원본의 메타데이터를 비운다.
     * @pre 현재 메시지는 빈 상태여야 한다.
     * @details 힙 페이로드는 포인터를 이전하고 인라인 페이로드는 이동 콜백으로 생성한다. 원본이 비어 있으면 아무 작업도 하지 않는다.
     */
    void MoveFrom(FMessage&& Other) noexcept;

    /**
     * @brief 페이로드 자체를 소멸시키지 않고 타입, 데이터, 콜백 포인터와 힙 할당 상태를 초기화한다.
     */
    void ClearMetadata() noexcept;

private:
    /**
     * @brief 페이로드 타입을 식별하는 외부 정적 타입 정보의 포인터.
     */
    const FMessageTypeInfo* mType{nullptr};

    /**
     * @brief 현재 소유하는 페이로드의 주소. 빈 메시지이면 nullptr이다.
     */
    void* mData{nullptr};

    /**
     * @brief 현재 페이로드의 소멸 및 메모리 해제를 담당하는 콜백.
     */
    FDestroyFunction mDestroyFunction{nullptr};

    /**
     * @brief 인라인 페이로드의 이동 콜백. 힙 저장 시에는 사용하지 않는다.
     */
    FMoveFunction mMoveFunction{nullptr};

    /**
     * @brief 페이로드가 힙 할당을 사용하는지 나타내는 상태.
     */
    bool mHeapAllocated{false};

    /**
     * @brief 작은 메시지를 직접 생성하는 정렬된 내부 저장소.
     */
    alignas(InlineAlignment) std::byte mInlineStorage[InlineSize]{};
};

template <typename T, typename... Args>
    requires CMessageConstructible<T, Args...>
void FMessage::TryEmplace(Args&&... Arguments) {
    FMessage::Reset();

    if constexpr (CanStoreInline<T>()) {
        T* Value{std::construct_at(reinterpret_cast<T*>(mInlineStorage), std::forward<Args>(Arguments)...)};

        mType = &T::StaticTypeInfo();
        mData = Value;
        mDestroyFunction = &FMessage::DestroyInline<T>;
        mMoveFunction = &FMessage::MoveInline<T>;
        mHeapAllocated = false;
    } else {
        void* Allocation{Memory::Allocate(sizeof(T), alignof(T), Memory::EMemoryTag::Message)};
        T* Value{std::construct_at(static_cast<T*>(Allocation), std::forward<Args>(Arguments)...)};

        mType = &T::StaticTypeInfo();
        mData = Value;
        mDestroyFunction = &FMessage::DestroyHeap<T>;
        // 포인터 교체만 하면 되니까 움직일 필요가 없다.
        mMoveFunction = nullptr;
        mHeapAllocated = true;
    }
}

template <CMessageType T>
[[nodiscard]] bool FMessage::Is() const noexcept {
    return mType == &T::StaticTypeInfo();
}

template <CMessageType T>
[[nodiscard]] T* FMessage::Get() noexcept {
    return Is<T>() ? static_cast<T*>(mData) : nullptr;
}

template <CMessageType T>
[[nodiscard]] const T* FMessage::Get() const noexcept {
    return Is<T>() ? static_cast<const T*>(mData) : nullptr;
}

template <typename T>
constexpr bool FMessage::CanStoreInline() noexcept {
    return sizeof(T) <= InlineSize && alignof(T) <= InlineAlignment;
}

template <typename T>
void FMessage::DestroyInline(FMessage& Message) noexcept {
    std::destroy_at(static_cast<T*>(Message.mData));
}

template <typename T>
void FMessage::DestroyHeap(FMessage& Message) noexcept {
    T* Value{static_cast<T*>(Message.mData)};

    std::destroy_at(Value);
    Memory::Free(Value);
}

template <typename T>
void FMessage::MoveInline(FMessage& Destination, FMessage& Source) noexcept {
    T* SourceValue{static_cast<T*>(Source.mData)};
    T* DestinationValue{std::construct_at(reinterpret_cast<T*>(Destination.mInlineStorage), std::move(*SourceValue))};

    std::destroy_at(SourceValue);

    Destination.mData = DestinationValue;
    Source.mData = nullptr;
}
