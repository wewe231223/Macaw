#pragma once

#include "Core/Types.h"

namespace Renderer::GBuffer {

    inline constexpr std::size_t AttachmentCount{6};
    inline constexpr Core::Uint32 ReceivesDecals{1};
    inline constexpr Core::Uint32 Surface{2};
    inline constexpr Core::Uint32 Prelit{4};

}
