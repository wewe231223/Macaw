#pragma once

#include "Core/Base/FAssetHandle.h"
#include "Asset/UFont.h"
#include "Asset/Pipeline/UPipeline.h"

class IAssetRegistryMutator {
public:
    virtual ~IAssetRegistryMutator() = default;

public:
    virtual void SetPipelineRenderMode(FAssetHandle Handle, ERenderMode Mode) = 0;
    virtual const FFontGlyph* GetOrCreateFontGlyph(FAssetHandle Handle, char32_t CodePoint) = 0;
};
