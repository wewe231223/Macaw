#include "FMessage.h"

FMessage::FMessage(FMessage&& Other) noexcept {
	FMessage::MoveFrom(std::move(Other));
}

FMessage& FMessage::operator=(FMessage&& Other) noexcept {
	if (this != &Other) {
		FMessage::Reset();
		FMessage::MoveFrom(std::move(Other));
	}

	return *this; 
}

FMessage::~FMessage() noexcept {
	FMessage::Reset(); 
}

bool FMessage::IsValid() const noexcept {
	return mData != nullptr;
}

const FTypeInfo& FMessage::GetTypeInfo() const noexcept {
	assert(mType != nullptr); // ? 컴파일이 안될텐데 
	return *mType;
}

void FMessage::Reset() noexcept {
	if (mData != nullptr) {
		assert(mDestroyFunction != nullptr);
		mDestroyFunction(*this);
	}

	ClearMetadata();
}

void FMessage::MoveFrom(FMessage&& Other) noexcept {
    if (!Other.IsValid()) {
        return;
    }

    mType = Other.mType;
    mDestroyFunction = Other.mDestroyFunction;
    mMoveFunction = Other.mMoveFunction;
    mHeapAllocated = Other.mHeapAllocated;

    if (mHeapAllocated) {
        mData = std::exchange(Other.mData, nullptr);
        Other.ClearMetadata();
        return;
    }

    assert(mMoveFunction != nullptr);
    mMoveFunction(*this, Other);
    Other.ClearMetadata();
}

void FMessage::ClearMetadata() noexcept {
	mType = nullptr;
	mData = nullptr;
	mDestroyFunction = nullptr;
	mMoveFunction = nullptr;
	mHeapAllocated = false;
}








