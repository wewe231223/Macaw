#pragma once

#include "Engine/EViewMode.h"

#include <array>

namespace Editor::BufferVisualization {

    inline constexpr std::array Modes{
        Engine::EViewMode::Lit_Phong,
        Engine::EViewMode::BaseColor,
        Engine::EViewMode::WorldNormal,
        Engine::EViewMode::Specular,
        Engine::EViewMode::Emissive,
        Engine::EViewMode::AmbientColor,
        Engine::EViewMode::Reflection,
        Engine::EViewMode::BaseColorAlpha,
        Engine::EViewMode::Shininess,
        Engine::EViewMode::ReceiveShadows,
        Engine::EViewMode::SceneDepth,
        Engine::EViewMode::SceneStencil
    };

    inline constexpr std::array<const char*, Modes.size()> Names{
        "Scene Color",
        "Base Color",
        "World Normal",
        "Specular",
        "Emissive",
        "Ambient Color",
        "Reflection",
        "Base Color Alpha",
        "Shininess",
        "Receive Shadows",
        "Scene Depth",
        "Scene Stencil"
    };

}
