/**
 * @file FAssetDragDrop.h
 * @brief 에셋 종류별 드래그 페이로드 식별자를 정의합니다.
 * @ingroup Editor
 */

#pragma once

namespace EditorAssetDragDrop {
    /**
     * @brief 메시 에셋 경로를 전달하는 드래그 페이로드 식별자입니다.
     */
    inline constexpr char MeshPayload[]{"MACAW_MESH_ASSET"};
    /**
     * @brief 머티리얼 에셋 경로를 전달하는 드래그 페이로드 식별자입니다.
     */
    inline constexpr char MaterialPayload[]{"MACAW_MATERIAL_ASSET"};
    /**
     * @brief 텍스처 에셋 경로를 전달하는 드래그 페이로드 식별자입니다.
     */
    inline constexpr char TexturePayload[]{"MACAW_TEXTURE_ASSET"};
}
