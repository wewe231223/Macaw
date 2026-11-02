#include "FShaderInstance.h"

#include "FObjectConstants.h"

#include <utility>

namespace Renderer {

    Renderer::FShaderInstance::FShaderInstance(std::shared_ptr<const RenderCore::FShader> Code, Renderer::EMeshPass Pass, bool Instanced, bool Skinned)
		: mCode{std::move(Code)},
		  mPass{Pass},
		  mInstanced{Instanced && !Skinned},
		  mSkinned{Skinned} {
    }

    bool Renderer::FShaderInstance::IsCurrent() const {
        return HasResources() && mRevision == mCode->GetRevision();
    }

    bool Renderer::FShaderInstance::HasResources() const {
        return mVertexShader != nullptr ? mInputLayout != nullptr : mPixelShader != nullptr;
    }

    Core::Uint64 Renderer::FShaderInstance::GetRevision() const {
        return mRevision;
    }

    bool Renderer::FShaderInstance::CreateResources(ID3D11Device* Device, Core::FString& Error) {
        Error.clear();

        if (IsCurrent()) {
            return true;
        }

        if (Device == nullptr || mCode == nullptr) {
            Error = "Invalid shader instance";
            return false;
        }

        const auto Compilation{mCode->GetCompilation()};
        const std::span<const Core::Uint8> Bytes{Compilation->mByteCode};

        if (mCode->GetDescription().mTargetProfile.starts_with("ps_")) {
            Microsoft::WRL::ComPtr<ID3D11PixelShader> Pixel{};

            if (FAILED(Device->CreatePixelShader(Bytes.data(), Bytes.size(), nullptr, Pixel.GetAddressOf()))) {
                Error = "Cannot create the pixel shader";
                return false;
            }

            mPixelShader = std::move(Pixel);
            mRevision = mCode->GetRevision();
            return true;
        }

        if (!mCode->GetDescription().mTargetProfile.starts_with("vs_")) {
            Error = "Unsupported shader instance stage";
            return false;
        }

        const bool DepthOnly{mPass == Renderer::EMeshPass::ShadowDepth};
        Core::TArray<D3D11_INPUT_ELEMENT_DESC> Elements{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};

        // 본 인덱스(uint4)와 가중치(float4)는 USkeletalMesh::CreateResources가 만든 별도 정점 버퍼에서 5번 슬롯으로 읽습니다.
        if (mSkinned) {
            Elements.insert(Elements.end(), {{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 5, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 5, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}});
        }

        if (!DepthOnly) {
            Elements.insert(Elements.end(), {{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}});
        }

        if (mInstanced) {
            for (UINT Row{}; Row < 4; ++Row) {
                Elements.push_back({"INSTANCE_WORLD", Row, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, static_cast<UINT>(offsetof(Renderer::FObjectConstants, mWorld)) + Row * 16, D3D11_INPUT_PER_INSTANCE_DATA, 1});

                if (!DepthOnly) {
                    Elements.push_back({"INSTANCE_INVERSE", Row, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, static_cast<UINT>(offsetof(Renderer::FObjectConstants, mWorldInverse)) + Row * 16, D3D11_INPUT_PER_INSTANCE_DATA, 1});
                }
            }

            if (!DepthOnly) {
                Elements.push_back({"INSTANCE_UV", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, static_cast<UINT>(offsetof(Renderer::FObjectConstants, mUVTransform)), D3D11_INPUT_PER_INSTANCE_DATA, 1});
            }
        }

        Microsoft::WRL::ComPtr<ID3D11VertexShader> Vertex{};
        Microsoft::WRL::ComPtr<ID3D11InputLayout> Layout{};

        if (FAILED(Device->CreateVertexShader(Bytes.data(), Bytes.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreateInputLayout(Elements.data(), static_cast<UINT>(Elements.size()), Bytes.data(), Bytes.size(), Layout.GetAddressOf()))) {
            Error = "Cannot create the vertex shader or input layout";
            return false;
        }

        mVertexShader = std::move(Vertex);
        mInputLayout = std::move(Layout);
        mRevision = mCode->GetRevision();
        return true;
    }

    void Renderer::FShaderInstance::Bind(ID3D11DeviceContext* Context) const {
        if (mVertexShader != nullptr) {
            Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
            Context->IASetInputLayout(mInputLayout.Get());
        } else {
            Context->PSSetShader(mPixelShader.Get(), nullptr, 0);
        }
    }

}
