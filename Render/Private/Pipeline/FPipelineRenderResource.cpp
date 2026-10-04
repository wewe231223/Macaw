#include "pch.h"
#include "Render/Pipeline/FPipelineRenderResource.h"
#include "Pipeline/FD3D11Conversions.h"

bool FPipelineRenderResource::Initialize(ID3D11Device* Device, const UPipeline& Pipeline) {
    Reset();

    if (Device == nullptr) {
        return false;
    }

    mPipelines.resize(static_cast<std::size_t>(ERenderMode::Max));

    for (std::size_t Index{}; Index < mPipelines.size(); ++Index) {
        const FPipelineDescription* Description{Pipeline.GetDescription(static_cast<ERenderMode>(Index))};

        if (Description != nullptr && !Make(Device, *Description, mPipelines[Index])) {
            Reset();
            return false;
        }
    }

    return Pipeline.RenderModeSettable(ERenderMode::Lit);
}

bool FPipelineRenderResource::Make(ID3D11Device* Device, const FPipelineDescription& Description, FPipelineState& Pipeline) {
    if (Device == nullptr) {
        ErrorHandler::Report("Pipeline::Initialize", "A valid Direct3D device is required to initialize a pipeline.", ErrorHandler::EErrorLevel::Error);
        return false;
    }

    if (!Pipeline.mVertexShader.Initialize(Device, Description.mVertexShader)) {
        return false;
    }

    if (!Pipeline.mPixelShader.Initialize(Device, Description.mPixelShader)) {
        return false;
    }

    if (Description.mBHasGeometryShader) {
        if (!Pipeline.mGeometryShader.Initialize(Device, Description.mGeometryShader)) {
            return false;
        }
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> NativeInputLayout{};

    NativeInputLayout.reserve(Description.mInputLayout.size());

    for (const FInputElementDescription& Source : Description.mInputLayout) {
        D3D11_INPUT_ELEMENT_DESC Element{};

        Element.SemanticName = Source.mSemanticName.c_str();
        Element.SemanticIndex = Source.mSemanticIndex;
        Element.Format = ConvertVertexFormat(Source.mFormat);
        Element.InputSlot = Source.mInputSlot;
        Element.AlignedByteOffset = Source.mAlignedByteOffset;
        Element.InputSlotClass = Source.mInputClassification == EInputClassification::PerInstance ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
        Element.InstanceDataStepRate = Source.mInstanceDataStepRate;

        NativeInputLayout.emplace_back(Element);
    }

    HRESULT Result{S_OK};

    if (!NativeInputLayout.empty()) {
        Result = Device->CreateInputLayout(NativeInputLayout.data(), static_cast<UINT>(NativeInputLayout.size()), Pipeline.mVertexShader.GetByteCodeData(), Pipeline.mVertexShader.GetByteCodeSize(), Pipeline.mInputLayout.GetAddressOf());

        if (FAILED(Result)) {
            ErrorHandler::ReportHRESULT(Result, "Pipeline::Initialize", "Failed to create the input layout.", ErrorHandler::EErrorLevel::Error);
            Reset();
            return false;
        }
    } else {
        Pipeline.mInputLayout.Reset();
    }

    D3D11_RASTERIZER_DESC RasterizerDesc{};

    RasterizerDesc.FillMode = ConvertFillMode(Description.mRasterizer.mFillMode);
    RasterizerDesc.CullMode = ConvertCullMode(Description.mRasterizer.mCullMode);
    RasterizerDesc.FrontCounterClockwise = Description.mRasterizer.mFrontCounterClockwise;
    RasterizerDesc.DepthClipEnable = Description.mRasterizer.mDepthClipEnable;
    RasterizerDesc.ScissorEnable = Description.mRasterizer.mScissorEnable;

    Result = Device->CreateRasterizerState(&RasterizerDesc, Pipeline.mRasterizerState.GetAddressOf());

    if (FAILED(Result)) {
        ErrorHandler::ReportHRESULT(Result, "Pipeline::Initialize", "Failed to create the rasterizer state.", ErrorHandler::EErrorLevel::Error);
        Reset();
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC DepthStencilDesc{};

    DepthStencilDesc.DepthEnable = Description.mDepthStencil.mDepthEnable;
    DepthStencilDesc.DepthWriteMask = Description.mDepthStencil.mDepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    DepthStencilDesc.DepthFunc = ConvertCompareFunc(Description.mDepthStencil.mDepthFunc);
    DepthStencilDesc.StencilEnable = Description.mDepthStencil.mStencilEnable;
    DepthStencilDesc.StencilReadMask = Description.mDepthStencil.mStencilReadMask;
    DepthStencilDesc.StencilWriteMask = Description.mDepthStencil.mStencilWriteMask;
    DepthStencilDesc.FrontFace.StencilFunc = ConvertCompareFunc(Description.mDepthStencil.mStencilFunc);
    DepthStencilDesc.FrontFace.StencilPassOp = ConvertStencillOp(Description.mDepthStencil.mStencilPassOp);
    DepthStencilDesc.FrontFace.StencilFailOp = ConvertStencillOp(Description.mDepthStencil.mStencilFailOp);
    DepthStencilDesc.FrontFace.StencilDepthFailOp = ConvertStencillOp(Description.mDepthStencil.mStencilDepthFailOp);

    DepthStencilDesc.BackFace = DepthStencilDesc.FrontFace;

    Result = Device->CreateDepthStencilState(&DepthStencilDesc, Pipeline.mDepthStencilState.GetAddressOf());

    if (FAILED(Result)) {
        ErrorHandler::ReportHRESULT(Result, "Pipeline::Initialize", "Failed to create the depth-stencil state.", ErrorHandler::EErrorLevel::Error);
        Reset();
        return false;
    }

    D3D11_BLEND_DESC BlendDesc{};

    BlendDesc.AlphaToCoverageEnable = false;
    BlendDesc.IndependentBlendEnable = false;

    D3D11_RENDER_TARGET_BLEND_DESC& RenderTarget{BlendDesc.RenderTarget[0]};

    RenderTarget.BlendEnable = Description.mBlend.mBlendEnable;
    RenderTarget.SrcBlend = ConvertBlend(Description.mBlend.mSrcBlend);
    RenderTarget.DestBlend = ConvertBlend(Description.mBlend.mDestBlend);
    RenderTarget.BlendOp = ConvertBlendOp(Description.mBlend.mBlendOp);
    RenderTarget.SrcBlendAlpha = ConvertBlend(Description.mBlend.mSrcBlendAlpha);
    RenderTarget.DestBlendAlpha = ConvertBlend(Description.mBlend.mDestBlendAlpha);
    RenderTarget.BlendOpAlpha = ConvertBlendOp(Description.mBlend.mBlendOpAlpha);
    RenderTarget.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Result = Device->CreateBlendState(&BlendDesc, Pipeline.mBlendState.GetAddressOf());

    if (FAILED(Result)) {
        ErrorHandler::ReportHRESULT(Result, "Pipeline::Initialize", "Failed to create the blend state.", ErrorHandler::EErrorLevel::Error);
        Reset();
        return false;
    }

    Pipeline.mPrimitiveTopology = ConvertPrimitiveTopology(Description.mPrimitiveTopology);

    Pipeline.mInitialized = true;

    return true;
}

void FPipelineRenderResource::Bind(ID3D11DeviceContext* Context, ERenderMode Mode, UINT StencilReference) const {
    if (Context == nullptr) {
        ErrorHandler::Report("Pipeline::Bind", "A valid Direct3D device context is required to bind a pipeline.", ErrorHandler::EErrorLevel::Error);
        return;
    }

    const std::size_t Index{static_cast<std::size_t>(Mode)};

    if (Index >= mPipelines.size() || !mPipelines[Index].mInitialized) {
        return;
    }

    Context->IASetInputLayout(mPipelines[Index].mInputLayout.Get());
    Context->IASetPrimitiveTopology(mPipelines[Index].mPrimitiveTopology);

    Context->VSSetShader(mPipelines[Index].mVertexShader.GetVertexShader(), nullptr, 0);
    Context->PSSetShader(mPipelines[Index].mPixelShader.GetPixelShader(), nullptr, 0);

    Context->GSSetShader(mPipelines[Index].mGeometryShader.GetGeometryShader(), nullptr, 0);
    Context->HSSetShader(nullptr, nullptr, 0);
    Context->DSSetShader(nullptr, nullptr, 0);

    Context->RSSetState(mPipelines[Index].mRasterizerState.Get());
    Context->OMSetBlendState(mPipelines[Index].mBlendState.Get(), nullptr, 0xffffffff);
    Context->OMSetDepthStencilState(mPipelines[Index].mDepthStencilState.Get(), StencilReference);
}

void FPipelineRenderResource::Reset() {
    mPipelines.clear();
}
