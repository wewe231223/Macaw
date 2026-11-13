/**
 * @file BufferVisualization.h
 * @brief 버퍼 시각화 메뉴의 모드와 표시 이름 대응을 정의합니다.
 * @ingroup Editor
 */

#pragma once

#include "Engine/EViewMode.h"

#include <array>

/**
 * @brief 버퍼 시각화 메뉴의 렌더 모드와 표시 이름 대응입니다.
 */
namespace Editor::BufferVisualization {

    /**
     * @brief 버퍼 시각화 메뉴에 표시할 렌더 모드 목록입니다.
     */
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

    /**
     * @brief 시각화 모드와 같은 순서로 배치한 메뉴 표시 이름입니다.
     */
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
