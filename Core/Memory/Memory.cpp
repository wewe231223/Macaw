#include "pch.h"
#include "Memory.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>

namespace
{
	struct FAllocationHeader
	{
		void* RawPointer = nullptr;
		std::size_t Size = 0;
		std::size_t Alignment = 0;
		Memory::EMemoryTag Tag = Memory::EMemoryTag::Unknown;
	};

	struct FMemoryState
	{
		Memory::FMemoryStats Stats;
	};

	FMemoryState& GetMemoryState()
	{
		static FMemoryState State;
		return State;
	}
}


void* Memory::Allocate(std::size_t Size, std::size_t Alignment, EMemoryTag Tag)
{
		//Alignment는 0이 아닌 2의 거듭제곱이어야 한다.
		if (Alignment == 0 || (Alignment & (Alignment - 1)) != 0)
		{
			throw std::invalid_argument("Invalid memory alignment");
		}

		// 사용자 메모리와 Header 양쪽의 정렬 조건을 만족시킨다.
		const std::size_t EffectiveAlignment = (std::max)(Alignment, alignof(FAllocationHeader));

		const std::size_t PayloadSize = Size == 0 ? 1 : Size; // 0바이트 할당을 방지

		const std::size_t MaxSize = std::numeric_limits<std::size_t>::max();

		//header 크기와 정렬 여유 공간 계산 시 오버플로 검사
		if (EffectiveAlignment - 1 > MaxSize - sizeof(FAllocationHeader))
		{
			throw std::bad_alloc();
		}

		const std::size_t Overhead = sizeof(FAllocationHeader) + (EffectiveAlignment - 1);

		// 사용자 공간을 더했을 때 오버플로 검사
		if (PayloadSize > MaxSize - Overhead)
		{
			throw std::bad_alloc();
		}
		
		const std::size_t TotalSize = Overhead + PayloadSize;

		//실제 메모리를 넉넉하게 확보
		void* RawPointer = ::operator new(TotalSize);

		// header가 들어갈 공간을 먼저 건너뜀
		void* UserPointer = static_cast<std::byte*>(RawPointer) + sizeof(FAllocationHeader);

		std::size_t Space =
			TotalSize - sizeof(FAllocationHeader);

		// UserPointer를 요구된 Alignment에 맞는 주소로 이동시킨다.
		if (std::align(
			EffectiveAlignment,
			PayloadSize,
			UserPointer,
			Space) == nullptr)
		{
			::operator delete(RawPointer);
			throw std::bad_alloc();
		}

		// 사용자 메모리 바로 앞의 주소를 Header 위치로 사용한다.
		void* HeaderAddress =
			static_cast<std::byte*>(UserPointer)
			- sizeof(FAllocationHeader);

		// 이미 확보한 메모리 위에 Header 객체를 생성한다.
		::new (HeaderAddress) FAllocationHeader
		{
			RawPointer,
			Size,
			Alignment,
			Tag
		};

		FMemoryState& State = GetMemoryState();

		State.Stats.AllocatedBytes += Size;
		++State.Stats.ActiveAllocationCount;
		++State.Stats.TotalAllocationCount;

		State.Stats.PeakAllocatedBytes =
			(std::max)(
				State.Stats.PeakAllocatedBytes,
				State.Stats.AllocatedBytes);

		return UserPointer;
}

void Memory::Free(void* Ptr) noexcept
{
	if (Ptr == nullptr)
	{
		return;
	}

	// 사용자 주소 바로 앞에 저장된 Header를 찾는다.
	auto* Header = reinterpret_cast<FAllocationHeader*>(static_cast<std::byte*>(Ptr) - sizeof(FAllocationHeader));

	//메모리를 해제하기 전에 필요한 정보를 복사
	void* RawPointer = Header->RawPointer;
	const std::size_t Size = Header->Size;

	FMemoryState& State = GetMemoryState();

	// 현재 살아 있는 메모리와 할당 개수를 감소시킴
	State.Stats.AllocatedBytes -= Size;
	--State.Stats.ActiveAllocationCount;

	// Header 객체의 수명을 끝낸다.
	Header->~FAllocationHeader();

	// Header가 차지한 메모리를 해제한다.
	::operator delete(RawPointer);
}

Memory::FMemoryStats Memory::GetStats()
{
	return GetMemoryState().Stats;
}
