#include "pch.h"
#include "Memory.h"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>

namespace {
struct FAllocationHeader {
    void* mRawPointer{nullptr};
    std::size_t mSize{0};
    std::size_t mAlignment{0};
    Memory::EMemoryTag mTag{Memory::EMemoryTag::Unknown};
};

struct FMemoryState {
    Memory::FMemoryStats mStats{};
};

FMemoryState& GetMemoryState() {
    static FMemoryState State{};
    return State;
}

FString Str{};
}

const char* Memory::GetMemoryTagName(EMemoryTag Tag) {
    switch (Tag) {
        case EMemoryTag::Unknown:
            return "Unknown";
        case EMemoryTag::UObject:
            return "UObject";
        case EMemoryTag::Container:
            return "Container";
        case EMemoryTag::String:
            return "String";
        case EMemoryTag::Message:
            return "Message";
        default:
            return "Invalid";
    }
}

void* Memory::Allocate(std::size_t Size, std::size_t Alignment, EMemoryTag Tag) {
    if (Alignment == 0 || (Alignment & (Alignment - 1)) != 0) {
        throw std::invalid_argument("Invalid memory alignment");
    }

    const std::size_t EffectiveAlignment{(std::max)(Alignment, alignof(FAllocationHeader))};
    const std::size_t PayloadSize{Size == 0 ? 1 : Size};
    const std::size_t MaxSize{std::numeric_limits<std::size_t>::max()};

    if (EffectiveAlignment - 1 > MaxSize - sizeof(FAllocationHeader)) {
        throw std::bad_alloc();
    }

    const std::size_t Overhead{sizeof(FAllocationHeader) + (EffectiveAlignment - 1)};

    if (PayloadSize > MaxSize - Overhead) {
        throw std::bad_alloc();
    }

    const std::size_t TotalSize{Overhead + PayloadSize};
    void* RawPointer{::operator new(TotalSize)};
    void* UserPointer{static_cast<std::byte*>(RawPointer) + sizeof(FAllocationHeader)};

    std::size_t Space{TotalSize - sizeof(FAllocationHeader)};

    if (std::align(EffectiveAlignment, PayloadSize, UserPointer, Space) == nullptr) {
        ::operator delete(RawPointer);
        throw std::bad_alloc();
    }

    void* HeaderAddress{static_cast<std::byte*>(UserPointer) - sizeof(FAllocationHeader)};

    ::new (HeaderAddress) FAllocationHeader{ RawPointer, Size, Alignment, Tag};

    FMemoryState& State{GetMemoryState()};

    // 전체 메모리 통계 갱신
    State.mStats.mAllocatedBytes += Size;
    ++State.mStats.mActiveAllocationCount;
    ++State.mStats.mTotalAllocationCount;

    State.mStats.mPeakAllocatedBytes = (std::max)(State.mStats.mPeakAllocatedBytes, State.mStats.mAllocatedBytes);

    // 태그별 메모리 통계 갱신
    const std::size_t TagIndex{static_cast<std::size_t>(Tag)};
    if (TagIndex < static_cast<std::size_t>(EMemoryTag::Count)) {
        State.mStats.mTagStats[TagIndex].mAllocatedBytes += Size;
        ++State.mStats.mTagStats[TagIndex].mActiveAllocationCount;
    }

    return UserPointer;
}

void Memory::Free(void* Ptr) noexcept {
    if (Ptr == nullptr) {
        return;
    }

    auto* Header{reinterpret_cast<FAllocationHeader*>(static_cast<std::byte*>(Ptr) - sizeof(FAllocationHeader))};

    void* RawPointer{Header->mRawPointer};
    const std::size_t Size{Header->mSize};
    const EMemoryTag Tag{Header->mTag};

    FMemoryState& State{GetMemoryState()};

    // 전체 메모리 통계 감소
    State.mStats.mAllocatedBytes -= Size;
    --State.mStats.mActiveAllocationCount;
    ++State.mStats.mTotalDeallocationCount;

    // 태그별 메모리 통계 감소
    const std::size_t TagIndex{static_cast<std::size_t>(Tag)};
    if (TagIndex < static_cast<std::size_t>(EMemoryTag::Count)) {
        State.mStats.mTagStats[TagIndex].mAllocatedBytes -= Size;
        --State.mStats.mTagStats[TagIndex].mActiveAllocationCount;
    }

    Header->~FAllocationHeader();
    ::operator delete(RawPointer);
}

Memory::FMemoryStats Memory::GetStats() {
    return GetMemoryState().mStats;
}
