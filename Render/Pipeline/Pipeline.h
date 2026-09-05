#pragma once

#include "Shader.h"

#include <d3d11.h>
#include <wrl/client.h>

#include <filesystem>
#include <string>
#include <vector>

#include "Defines.h"
#include "Wrapper.h"

class Pipeline {
public:
    Pipeline() = default;
    ~Pipeline() = default;

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    Pipeline(Pipeline&&) noexcept = default;
    Pipeline& operator=(Pipeline&&) noexcept = default;

public:
    bool Initialize(ID3D11Device* Device, const FPipelineDescription& Description);
    bool Initialize(ID3D11Device* Device, const std::filesystem::path& OptionFile);

    void Bind(ID3D11DeviceContext* Context) const;
    void Reset();

private:
    bool LoadPipelineDescription(const std::filesystem::path& Path, FPipelineDescription& OutDescription);

private:
    Shader VertexShader{};
    Shader PixelShader{};

    Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> RasterizerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> BlendState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;

    D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};