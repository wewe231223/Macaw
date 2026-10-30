/**
 * @file Memory.cpp
 * @brief 정렬된 UObject 메모리 할당과 통계를 제공합니다.
 * @ingroup Core
 */

#include "Core/Memory.h"
#include "FAllocationHeader.h"
#include "Core/FMemoryStats.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <new>

namespace Core {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief UObject 수와 추적 중인 힙 사용량 통계입니다.
         */
        Core::FMemoryStats Stats{};
    }

    /**
     * @brief 요청한 정렬을 만족하는 힙 블록을 할당하고 사용량을 기록합니다.
     */
    void* Core::FMemory::Allocate(std::size_t Size, std::size_t Alignment) {
        const std::size_t EffectiveAlignment{std::max(Alignment, alignof(Core::FAllocationHeader))};

        if (EffectiveAlignment == 0 || (EffectiveAlignment & (EffectiveAlignment - 1)) != 0 || Size > std::numeric_limits<std::size_t>::max() - EffectiveAlignment - sizeof(Core::FAllocationHeader)) {
            Core::ErrorHandler::Fatal("Memory allocation", "Invalid allocation size or alignment");
        }

        const std::size_t Total{Size + EffectiveAlignment + sizeof(Core::FAllocationHeader)};
        void* Raw{::operator new(Total)};
        void* Pointer{static_cast<std::byte*>(Raw) + sizeof(Core::FAllocationHeader)};
        std::size_t Space{Total - sizeof(Core::FAllocationHeader)};

        if (std::align(EffectiveAlignment, std::max<std::size_t>(Size, 1), Pointer, Space) == nullptr) {
            ::operator delete(Raw);
            Core::ErrorHandler::Fatal("Memory allocation", "Invalid allocation size or alignment");
        }

        auto* Header{std::construct_at(reinterpret_cast<Core::FAllocationHeader*>(static_cast<std::byte*>(Pointer) - sizeof(Core::FAllocationHeader)), Core::FAllocationHeader{Raw, Size})};

        Stats.mHeapBytes += Header->mSize;
        Stats.mPeakHeapBytes = std::max(Stats.mPeakHeapBytes, Stats.mHeapBytes);

        return Pointer;
    }

    /**
     * @brief 정렬된 블록의 원본 주소를 해제하고 추적 중인 사용량을 차감합니다.
     */
    void Core::FMemory::Free(void* Pointer) noexcept {
        if (Pointer == nullptr) {
            return;
        }

        auto* Header{reinterpret_cast<Core::FAllocationHeader*>(static_cast<std::byte*>(Pointer) - sizeof(Core::FAllocationHeader))};
        void* Raw{Header->mRaw};

        Stats.mHeapBytes -= Header->mSize;

        std::destroy_at(Header);
        ::operator delete(Raw);
    }

    /**
     * @brief 생존 객체와 누적 생성 객체 수를 증가시킵니다.
     */
    void Core::FMemory::ObjectCreated() {
        ++Stats.mLiveObjects;
        ++Stats.mCreatedObjects;
    }

    /**
     * @brief 생존 객체 수를 감소시킵니다.
     */
    void Core::FMemory::ObjectDestroyed() {
        --Stats.mLiveObjects;
    }

    /**
     * @brief 현재 UObject 개수와 추적 중인 힙 사용량의 스냅샷을 반환합니다.
     */
    Core::FMemoryStats Core::FMemory::GetStats() {
        return Stats;
    }

}
