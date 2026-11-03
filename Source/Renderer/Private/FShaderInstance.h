#pragma once

#include "EMeshPass.h"
#include "RenderCore/Shader/FShader.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    class FShaderInstance {
    public:
        FShaderInstance(std::shared_ptr<const RenderCore::FShader> Code, Renderer::EMeshPass Pass, bool Instanced, bool Skinned);

    public:
        bool IsCurrent() const;
        bool HasResources() const;
        Core::Uint64 GetRevision() const;

        bool CreateResources(ID3D11Device* Device, Core::FString& Error);
        void Bind(ID3D11DeviceContext* Context) const;

    private:
        std::shared_ptr<const RenderCore::FShader> mCode{};

        Renderer::EMeshPass mPass{};

        bool mInstanced{};

        bool mSkinned{};

        Core::Uint64 mRevision{};

        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};

        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};

        Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
    };

}
