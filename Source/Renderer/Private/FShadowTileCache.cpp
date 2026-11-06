/**
 * @file FShadowTileCache.cpp
 * @brief 그림자 아틀라스 타일의 깊이를 다시 그릴지 판단하는 캐시의 키와 스탬프를 정의합니다.
 * @ingroup Renderer
 */

#include "FShadowTileCache.h"

#include "Core/GuidHash.h"

namespace Renderer {

    /**
     * @brief 장면 번호·광원 GUID·면 번호를 섞어 해시를 계산합니다.
     */
    std::size_t Renderer::FShadowTileKeyHash::operator()(const Renderer::FShadowTileKey& Key) const noexcept {
        std::size_t Hash{Core::FGuidHash{}(Key.mLight)};

        Hash ^= std::hash<Core::Uint64>{}(Key.mScene.GetValue()) + 0x9E3779B97F4A7C15ull + (Hash << 6) + (Hash >> 2);
        Hash ^= std::hash<Core::Uint32>{}(Key.mFace) + 0x9E3779B97F4A7C15ull + (Hash << 6) + (Hash >> 2);

        return Hash;
    }

}
