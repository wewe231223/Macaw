#include "Core/Memory.h"
#include "FAllocationHeader.h"
#include "Core/FMemoryStats.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <new>

namespace {
    FMemoryStats Stats{};
}

void* FMemory::Allocate(std::size_t Size, std::size_t Alignment) {
    const std::size_t EffectiveAlignment{std::max(Alignment, alignof(FAllocationHeader))};

    if (EffectiveAlignment == 0 || (EffectiveAlignment & (EffectiveAlignment - 1)) != 0 || Size > std::numeric_limits<std::size_t>::max() - EffectiveAlignment - sizeof(FAllocationHeader)) {
        ErrorHandler::Fatal("Memory allocation", "Invalid allocation size or alignment");
    }

    const std::size_t Total{Size + EffectiveAlignment + sizeof(FAllocationHeader)};
    void* Raw{::operator new(Total)};
    void* Pointer{static_cast<std::byte*>(Raw) + sizeof(FAllocationHeader)};
    std::size_t Space{Total - sizeof(FAllocationHeader)};

    if (std::align(EffectiveAlignment, std::max<std::size_t>(Size, 1), Pointer, Space) == nullptr) {
        ::operator delete(Raw);
        ErrorHandler::Fatal("Memory allocation", "Invalid allocation size or alignment");
    }

    auto* Header{std::construct_at(reinterpret_cast<FAllocationHeader*>(static_cast<std::byte*>(Pointer) - sizeof(FAllocationHeader)), FAllocationHeader{Raw, Size})};

    Stats.mHeapBytes += Header->mSize;
    Stats.mPeakHeapBytes = std::max(Stats.mPeakHeapBytes, Stats.mHeapBytes);

    return Pointer;
}

void FMemory::Free(void* Pointer) noexcept {
    if (Pointer == nullptr) {
        return;
    }

    auto* Header{reinterpret_cast<FAllocationHeader*>(static_cast<std::byte*>(Pointer) - sizeof(FAllocationHeader))};
    void* Raw{Header->mRaw};

    Stats.mHeapBytes -= Header->mSize;

    std::destroy_at(Header);
    ::operator delete(Raw);
}

void FMemory::ObjectCreated() {
    ++Stats.mLiveObjects;
    ++Stats.mCreatedObjects;
}

void FMemory::ObjectDestroyed() {
    --Stats.mLiveObjects;
}

FMemoryStats FMemory::GetStats() {
    return Stats;
}
