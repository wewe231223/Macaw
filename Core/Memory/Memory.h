#pragma once
#include <cstddef>

namespace Memory {
    enum class EMemoryTag {
        Unknown,
        UObject,
        Container,
        String,
        Message,
        Count
    };

    // 태그 이름을 문자열로 반환하는 헬퍼 함수
    const char* GetMemoryTagName(EMemoryTag Tag);

    // 태그별 메모리 통계
    struct FTagStats {
        std::size_t mAllocatedBytes{0};
        std::size_t mActiveAllocationCount{0};
    };

    struct FMemoryStats {
        std::size_t mAllocatedBytes{0};
        std::size_t mPeakAllocatedBytes{0};
        std::size_t mActiveAllocationCount{0};
        std::size_t mTotalAllocationCount{0};
        std::size_t mTotalDeallocationCount{0};

        // EMemoryTag::Count 크기만큼의 태그별 통계 배열
        FTagStats mTagStats[static_cast<std::size_t>(EMemoryTag::Count)]{};
    };

    void* Allocate(std::size_t Size, std::size_t Alignment, EMemoryTag Tag = EMemoryTag::Unknown);

    void Free(void* Ptr) noexcept;

    FMemoryStats GetStats();
}
