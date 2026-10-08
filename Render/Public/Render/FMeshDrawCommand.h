#pragma once
#include "Render/ERenderPass.h"
#include "Render/FMeshDrawState.h"
#include "Render/FMeshDrawPipelineState.h"
#include "Asset/Pipeline/UPipeline.h"

#include <array>
#include <memory>

struct FMeshDrawCommand {
    FMeshDrawState mState{};
    Uint32 mMaterialIndex{};
    ERenderPass mPass{ERenderPass::Opaque};
    std::array<FMeshDrawPipelineState, static_cast<std::size_t>(ERenderMode::Max)> mPipelineStates{};
    std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, 4> mVertexBuffers{};
    std::array<Uint32, 4> mVertexStrides{};
    std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, MaxMaterialTextureFields> mTextures{};
};

struct FVisibleMeshDrawCommand {
    Uint32 mObjectIndex{};
    Uint32 mCommandIndex{};
    float mLODDither{};
    float mSortDepth{};
};

struct FMeshDrawCommandBatch {
    std::shared_ptr<const FMeshDrawCommand> mCommand{};
    Uint32 mFirstRecord{};
    Uint32 mRecordCount{};
    float mSortDepth{};
};
