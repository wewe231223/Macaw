#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FSceneView.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <span>

namespace Renderer {

    class FTileLightCulling {
    public:
        ReflectTypes(FTileLightCulling);

    public:
        ReflectStruct(FTileLightCulling);

        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode);
        bool Dispatch(ID3D11Device* Device, ID3D11DeviceContext* Context, ID3D11ShaderResourceView* Lights, const Engine::FSceneView& View, Core::Uint32 LightCount);
        std::array<ID3D11ShaderResourceView*, 2> GetViews() const;

    private:
        bool EnsureBuffer(ID3D11Device* Device, Core::Uint32 Index, Core::Uint64 Count, Core::Uint32 Stride);
        ReflectMembers();

    private:
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> mShader{};
        ReflectProperty(mShader, "Shader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
        ReflectProperty(mConstants, "Constants", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, 3> mBuffers{};
        ReflectProperty(mBuffers, "Buffers", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, 3> mViews{};
        ReflectProperty(mViews, "Views", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        std::array<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>, 3> mOutputs{};
        ReflectProperty(mOutputs, "Outputs", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        std::array<Core::Uint32, 3> mCapacities{};
        ReflectProperty(mCapacities, "Capacities", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
