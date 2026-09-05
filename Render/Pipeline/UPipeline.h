#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <filesystem>
#include <string>
#include <vector>

#include "FShader.h"

#include "Defines.h"
#include "../../Core/Base/TypeInfo.h"
#include "Wrapper.h"

class UPipeline : public UObject {
public:
    UPipeline() = default;
    ~UPipeline() = default;

    UPipeline(const UPipeline&) = delete;
    UPipeline& operator=(const UPipeline&) = delete;

    UPipeline(UPipeline&&) noexcept = default;
    UPipeline& operator=(UPipeline&&) noexcept = default;

public:
    bool Initialize(ID3D11Device* Device, const FPipelineDescription& Description);
    bool Initialize(ID3D11Device* Device, const std::filesystem::path& OptionFile);

    void Bind(ID3D11DeviceContext* Context) const;
    void Reset();

private:
    bool LoadPipelineDescription(const std::filesystem::path& Path, FPipelineDescription& OutDescription);

private:
    FShader VertexShader{};
    FShader PixelShader{};

    Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> RasterizerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> BlendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;

    D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};


