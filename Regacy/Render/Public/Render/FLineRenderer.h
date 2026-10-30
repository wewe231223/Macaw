#pragma once
#include "Core/Debug/ILineDrawContext.h"
#include "Render/Buffer/FGraphicsBuffer.h"
#include "Render/Pipeline/FPipelineRenderResource.h"

class FRenderBuffers;

enum class ERenderStream : Uint8;

class FLineRenderer final : public ILineDrawContext {
private:
    struct FQuadVertex {
        FVector2D mCorner{};
    };

    struct FLineInstance {
        FVector4 mStartAndWidth{};
        FVector4 mEndAndPadding{};
        FVector4 mColor{};
    };

    struct FLineBatch {
        TArray<FLineInstance> mInstances{};
    };

public:
    FLineRenderer() = default;
    ~FLineRenderer() override = default;

    FLineRenderer(const FLineRenderer&) = delete;

    FLineRenderer& operator=(const FLineRenderer&) = delete;

    FLineRenderer(FLineRenderer&&) noexcept = default;

    FLineRenderer& operator=(FLineRenderer&&) noexcept = default;

public:
    void Initialize(ID3D11Device* InDevice, Uint32 InitialLineCapacity = 1024);
    void Reset();

    void AddLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels = 1.0f, ELineDepthMode DepthMode = ELineDepthMode::DepthTested) override;
    void AddGridLine(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels, float GridSpacing, ELineDepthMode DepthMode);
    void AddRay(const FVector3& Origin, const FVector3& Direction, float Length, const FVector4& Color, float WidthPixels = 1.0f, ELineDepthMode DepthMode = ELineDepthMode::DepthTested);

    void Render(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers);
    void RenderOrientationAxis(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers);
    void Clear();

    [[nodiscard]] Uint32 GetLineCount() const;
    [[nodiscard]] bool IsEmpty() const;

private:
    void RenderLines(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, bool OrientationAxis);
    void AddLineInternal(const FVector3& Start, const FVector3& End, const FVector4& Color, float WidthPixels, ELineDepthMode DepthMode, float GridSpacing);
    bool CreateQuadGeometry(ID3D11Device* Device);
    bool RenderBatch(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, FLineBatch& Batch, const FPipelineRenderResource* Pipeline, ERenderStream Stream);

private:
    ID3D11Device* mDevice{nullptr};

    std::unique_ptr<FPipelineRenderResource> mDepthTestedPipeline{};
    std::unique_ptr<FPipelineRenderResource> mOverlayPipeline{};

    FGraphicsBuffer mQuadVertexBuffer{};

    FLineBatch mDepthTestedBatch{};
    FLineBatch mOverlayBatch{};
};
