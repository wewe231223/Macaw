#include "pch.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Memory/Memory.h"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>

namespace {
    struct FAllocationHeader {
        void* mRawPointer{nullptr};
        std::size_t mSize{0};
        std::size_t mAlignment{0};
        Memory::EMemoryTag mTag{Memory::EMemoryTag::Unknown};
    };

    FString Str{};
}

const char* Memory::GetMemoryTagName(EMemoryTag Tag) {
    return Stat::GetMemoryTagName(Tag);
}

void* Memory::Allocate(std::size_t Size, std::size_t Alignment, EMemoryTag Tag) {
    if (Alignment == 0 || (Alignment & (Alignment - 1)) != 0) {
        ErrorHandler::Report("Memory", "Invalid memory alignment", ErrorHandler::EErrorLevel::Critical);
    }

    const std::size_t EffectiveAlignment{(std::max)(Alignment, alignof(FAllocationHeader))};
    const std::size_t PayloadSize{Size == 0 ? 1 : Size};
    const std::size_t MaxSize{std::numeric_limits<std::size_t>::max()};

    if (EffectiveAlignment - 1 > MaxSize - sizeof(FAllocationHeader)) {
        ErrorHandler::Report("Memory::Allocate", "Allocation alignment exceeds the addressable size.", ErrorHandler::EErrorLevel::Critical);
    }

    const std::size_t Overhead{sizeof(FAllocationHeader) + (EffectiveAlignment - 1)};

    if (PayloadSize > MaxSize - Overhead) {
        ErrorHandler::Report("Memory::Allocate", "Allocation size exceeds the addressable size.", ErrorHandler::EErrorLevel::Critical);
    }

    const std::size_t TotalSize{Overhead + PayloadSize};
    void* RawPointer{::operator new(TotalSize, std::nothrow)};

    ErrorHandler::Report(RawPointer == nullptr, "Memory::Allocate", "Failed to allocate memory.", ErrorHandler::EErrorLevel::Critical);

    void* UserPointer{static_cast<std::byte*>(RawPointer) + sizeof(FAllocationHeader)};

    std::size_t Space{TotalSize - sizeof(FAllocationHeader)};

    if (std::align(EffectiveAlignment, PayloadSize, UserPointer, Space) == nullptr) {
        ::operator delete(RawPointer);
        ErrorHandler::Report("Memory::Allocate", "Failed to align allocated memory.", ErrorHandler::EErrorLevel::Critical);
    }

    void* HeaderAddress{static_cast<std::byte*>(UserPointer) - sizeof(FAllocationHeader)};

    ::new (HeaderAddress) FAllocationHeader{RawPointer, Size, Alignment, Tag};

    Stat::RecordAllocation(Size, Tag);

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

    Stat::RecordDeallocation(Size, Tag);

    Header->~FAllocationHeader();
    ::operator delete(RawPointer);
}

Memory::FMemoryStats Memory::GetStats() {
    return Stat::GetMemoryStats();
}
