#pragma once

#include <cstddef>

namespace Memory
{
	enum class EMemoryTag
	{
		Unknown,
		UObject,
		Container,
		String,
		Count
	};

	struct FMemoryStats
	{
		std::size_t AllocatedBytes = 0;
		std::size_t PeakAllocatedBytes = 0;
		std::size_t ActiveAllocationCount = 0;
		std::size_t TotalAllocationCount = 0;
	};

	void* Allocate(std::size_t Size, std::size_t Alignment, EMemoryTag Tag = EMemoryTag::Unknown);

	void Free(void* Ptr) noexcept;

	FMemoryStats GetStats();
}
