#include "FShadowAtlasAllocator.h"

#include <bit>

FShadowAtlasAllocator::FShadowAtlasAllocator(Uint32 Size)
    : mSize{Size} {
    Reset();
}

void FShadowAtlasAllocator::Reset() {
    mNodes.clear();
    mNodes.push_back({{0, 0, mSize}});
}

std::optional<FShadowAtlasBlock> FShadowAtlasAllocator::Allocate(Uint32 Size) {
    if (Size == 0 || Size > mSize || !std::has_single_bit(Size)) {
        return std::nullopt;
    }
    return AllocateNode(0, Size);
}

std::optional<FShadowAtlasBlock> FShadowAtlasAllocator::AllocateNode(Uint32 Index, Uint32 Size) {
    const FNode Node{mNodes[Index]};
    if (Node.mOccupied || Node.mBlock.mSize < Size) {
        return std::nullopt;
    }
    if (Node.mSplit) {
        for (Uint32 Child : Node.mChildren) {
            const auto Block{AllocateNode(Child, Size)};
            if (Block.has_value()) {
                return Block;
            }
        }
        return std::nullopt;
    }
    if (Node.mBlock.mSize == Size) {
        mNodes[Index].mOccupied = true;
        return Node.mBlock;
    }
    const Uint32 Half{Node.mBlock.mSize / 2};
    const Uint32 First{static_cast<Uint32>(mNodes.size())};
    mNodes[Index].mChildren = {First, First + 1, First + 2, First + 3};
    mNodes[Index].mSplit = true;
    for (Uint32 Child{}; Child < 4; ++Child) {
        mNodes.push_back({{Node.mBlock.mX + (Child & 1) * Half, Node.mBlock.mY + (Child >> 1) * Half, Half}});
    }
    return AllocateNode(First, Size);
}
