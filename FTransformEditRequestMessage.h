#pragma once

#include <atomic>
#include <cstdint>

#include "Core/Base/FObjectHandle.h"
#include "Core/Base/TypeInfo.h"
#include "FMath.h"

enum class ETransformEditPhase : std::uint8_t {
	Begin,
	Update,
	Commit,
	Cancel
};

inline std::uint64_t AcquireTransformEditSessionId() noexcept {
	static std::atomic_uint64_t NextSessionId{ 1 };
	return NextSessionId.fetch_add(1, std::memory_order_relaxed);
}


struct FTransformEditRequestMessage {
	inline static const FTypeInfo TypeInfo{
		"FTransformEditRequestMessage",
		nullptr,
		nullptr
	};

	static const FTypeInfo& StaticTypeInfo() noexcept {
		return TypeInfo;
	}

	std::uint64_t SessionId = 0;
	ETransformEditPhase Phase = ETransformEditPhase::Begin;
	FObjectHandle TargetHandle{};
	FMatrix DesiredWorld{ FMatrix::Identity };
	std::uint64_t ExpectedTransformRevision = 0;

	FTransformEditRequestMessage() = default;

	FTransformEditRequestMessage(std::uint64_t InSessionId, ETransformEditPhase InPhase, FObjectHandle InTargetHandle, const FMatrix& InDesiredWorld, std::uint64_t InExpectedTransformRevision) noexcept
		: SessionId(InSessionId), Phase(InPhase), TargetHandle(InTargetHandle), DesiredWorld(InDesiredWorld), ExpectedTransformRevision(InExpectedTransformRevision) {
	}
};
