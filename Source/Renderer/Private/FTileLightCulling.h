#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FSceneView.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <span>

namespace Renderer {

    class FTileLightCulling {
    public:
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode);
        bool Dispatch(ID3D11Device* Device, ID3D11DeviceContext* Context, ID3D11ShaderResourceView* Lights, const Engine::FSceneView& View, Core::Uint32 LightCount);
        std::array<ID3D11ShaderResourceView*, 2> GetViews() const;

    private:
        bool EnsureBuffer(ID3D11Device* Device, Core::Uint32 Index, Core::Uint64 Count, Core::Uint32 Stride);

    private:
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> mShader{};
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
        std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, 3> mBuffers{};
        std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, 3> mViews{};
        std::array<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>, 3> mOutputs{};
        std::array<Core::Uint32, 3> mCapacities{};
    };

}
