#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "Defines.h"

class FShader {
public:
    FShader() = default;
    ~FShader() = default;

    FShader(const FShader&) = delete;
    FShader& operator=(const FShader&) = delete;

    FShader(FShader&&) noexcept = default;
    FShader& operator=(FShader&&) noexcept = default;

public:
    bool Initialize(ID3D11Device* Device, const FShaderDescription& Description);
    void Reset();

public:
    EShaderStage GetStage() const noexcept;

    const void* GetByteCodeData() const noexcept;

    std::size_t GetByteCodeSize() const noexcept;

    ID3D11VertexShader* GetVertexShader() const noexcept;

    ID3D11PixelShader* GetPixelShader() const noexcept;

    ID3D11GeometryShader* GetGeometryShader() const noexcept;

    ID3D11HullShader* GetHullShader() const noexcept;

    ID3D11DomainShader* GetDomainShader() const noexcept;

    ID3D11ComputeShader* GetComputeShader() const noexcept;

private:
    EShaderStage mStage{EShaderStage::Vertex};
    std::vector<std::uint8_t> mByteCode{};

    Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> mGeometryShader{};
    Microsoft::WRL::ComPtr<ID3D11HullShader> mHullShader{};
    Microsoft::WRL::ComPtr<ID3D11DomainShader> mDomainShader{};
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> mComputeShader{};
};
