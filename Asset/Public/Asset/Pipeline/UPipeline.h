#pragma once

#include <filesystem>
#include <array>
#include <string>
#include <vector>
#include "RenderCore/Pipeline/FPipelineDescription.h"
#include "Asset/UAsset.h"
#include "CoreUObject/TypeInfo.h"

enum class ERenderMode : std::size_t {
    Lit,
    Outline,
    Unlit,
    Wireframe,
    Max
};

class UPipeline : public UAsset {
public:
    UPipeline() = default;
    ~UPipeline() = default;

    UPipeline(const UPipeline&) = delete;
    UPipeline& operator=(const UPipeline&) = delete;

    UPipeline(UPipeline&&) noexcept = default;
    UPipeline& operator=(UPipeline&&) noexcept = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UPipeline, UAsset);

    bool Initialize(const std::filesystem::path& PipelinePath);

    void Reset();

    void SetRenderMode(ERenderMode Mode);
    bool RenderModeSettable(ERenderMode Mode) const;
    ERenderMode GetRenderMode() const;
    ERenderMode ResolveRenderMode(ERenderMode Mode) const;

    const FPipelineDescription* GetDescription(ERenderMode Mode) const;
    Uint64 GetRenderRevision() const;


private:
    bool InitializeFamily(const std::filesystem::path& FamilyDirectory);
    bool InitializeModes(const std::array<std::filesystem::path, static_cast<std::size_t>(ERenderMode::Max)>& ModePaths);
    bool LoadPipelineDescription(const std::filesystem::path& Path, FPipelineDescription& OutDescription);

    virtual void Serialize(FArchive& Ar) override;

private:
    std::filesystem::path mOptionFilePath{};

    std::size_t mModeIndex{0};

    std::array<FPipelineDescription, static_cast<std::size_t>(ERenderMode::Max)> mDescriptions{};
    std::array<bool, static_cast<std::size_t>(ERenderMode::Max)> mEnabledModes{};

    Uint64 mRenderRevision{1};

    std::size_t mPrimaryIndex{0};
};
