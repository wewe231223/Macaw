#include "FShadowTileCache.h"

#include "Core/GuidHash.h"

namespace Renderer {

    std::size_t Renderer::FShadowTileKeyHash::operator()(const Renderer::FShadowTileKey& Key) const noexcept {
        std::size_t Hash{Core::FGuidHash{}(Key.mLight)};
        Hash ^= std::hash<Core::Uint64>{}(Key.mScene.GetValue()) + 0x9E3779B97F4A7C15ull + (Hash << 6) + (Hash >> 2);
        Hash ^= std::hash<Core::Uint32>{}(Key.mFace) + 0x9E3779B97F4A7C15ull + (Hash << 6) + (Hash >> 2);

        return Hash;
    }

}
