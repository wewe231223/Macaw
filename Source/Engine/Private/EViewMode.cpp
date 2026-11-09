#include "Engine/EViewMode.h"

namespace Engine {

    bool IsBufferVisualization(Engine::EViewMode Mode) {
        return (Mode >= Engine::EViewMode::WorldNormal && Mode <= Engine::EViewMode::ReceiveShadows) || Mode == Engine::EViewMode::SceneStencil;
    }

}
