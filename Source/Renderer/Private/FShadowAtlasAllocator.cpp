/**
 * @file FShadowAtlasAllocator.cpp
 * @brief 정사각형 영역을 사분할하여 그림자 아틀라스의 공간을 할당합니다.
 * @ingroup Renderer
 */

#include "Core/Types.h"
#include "FShadowAtlasAllocator.h"

#include <bit>

namespace Renderer {

    /**
     * @brief 두 블록의 픽셀 영역이 겹치는지 검사합니다.
     */
    bool Renderer::FShadowAtlasBlock::Overlaps(const Renderer::FShadowAtlasBlock& Other) const {
        return mX < Other.mX + Other.mSize && Other.mX < mX + mSize && mY < Other.mY + Other.mSize && Other.mY < mY + mSize;
    }

    /**
     * @brief 지정한 크기의 아틀라스 전체를 빈 영역으로 초기화합니다.
     */
    Renderer::FShadowAtlasAllocator::FShadowAtlasAllocator(Core::Uint32 Size)
        : mSize{Size} {
        Reset();
    }

    /**
     * @brief 모든 할당을 해제하고 아틀라스 전체를 하나의 빈 영역으로 되돌립니다.
     */
    void Renderer::FShadowAtlasAllocator::Reset() {
        mNodes.clear();
        mNodes.push_back({{0, 0, mSize}});
    }

    /**
     * @brief 요청한 크기의 비어 있는 정사각형 아틀라스 영역을 할당합니다.
     */
    std::optional<Renderer::FShadowAtlasBlock> Renderer::FShadowAtlasAllocator::Allocate(Core::Uint32 Size) {
        if (Size == 0 || Size > mSize || !std::has_single_bit(Size)) {
            return std::nullopt;
        }

        return AllocateNode(0, Size);
    }

    /**
     * @brief 지정한 노드에서 빈 영역을 찾고 필요하면 사분할하여 할당합니다.
     */
    std::optional<Renderer::FShadowAtlasBlock> Renderer::FShadowAtlasAllocator::AllocateNode(Core::Index_t Index, Core::Uint32 Size) {
        const FNode Node{mNodes[Index]};

        if (Node.mOccupied || Node.mBlock.mSize < Size) {
            return std::nullopt;
        }

        if (Node.mSplit) {
            for (Core::Index_t Child : Node.mChildren) {
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

        const Core::Uint32 Half{Node.mBlock.mSize / 2};
        const Core::Index_t First{static_cast<Core::Index_t>(mNodes.size())};

        mNodes[Index].mChildren = {First, First + 1, First + 2, First + 3};
        mNodes[Index].mSplit = true;

        for (Core::Index_t Child{}; Child < 4; ++Child) {
            mNodes.push_back({{Node.mBlock.mX + (Child & 1) * Half, Node.mBlock.mY + (Child >> 1) * Half, Half}});
        }

        return AllocateNode(First, Size);
    }

}
