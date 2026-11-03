#include "RenderCore/EShaderStage.h"

namespace RenderCore {

    RenderCore::EShaderStage operator|(RenderCore::EShaderStage Left, RenderCore::EShaderStage Right) {
        return static_cast<RenderCore::EShaderStage>(static_cast<Core::Uint8>(Left) | static_cast<Core::Uint8>(Right));
    }

    RenderCore::EShaderStage operator&(RenderCore::EShaderStage Left, RenderCore::EShaderStage Right) {
        return static_cast<RenderCore::EShaderStage>(static_cast<Core::Uint8>(Left) & static_cast<Core::Uint8>(Right));
    }

    bool HasStage(RenderCore::EShaderStage Stages, RenderCore::EShaderStage Stage) {
        return (Stages & Stage) != RenderCore::EShaderStage::None;
    }

}
