#pragma once

#include "EMeshPass.h"
#include "RenderCore/Shader/FShader.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    class FShaderInstance {
    public:
        ReflectTypes(FShaderInstance);

    public:
        FShaderInstance(std::shared_ptr<const RenderCore::FShader> Code, Renderer::EMeshPass Pass, bool Instanced, bool Skinned);

    public:
        ReflectStruct(FShaderInstance);

        bool IsCurrent() const;
        bool HasResources() const;
        Core::Uint64 GetRevision() const;

        bool CreateResources(ID3D11Device* Device, Core::FString& Error);
        void Bind(ID3D11DeviceContext* Context) const;

    private:
        ReflectMembers();

    private:
        std::shared_ptr<const RenderCore::FShader> mCode{};
        ReflectProperty(mCode, "Code", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        Renderer::EMeshPass mPass{};
        ReflectProperty(mPass, "Pass", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        bool mInstanced{};
        ReflectProperty(mInstanced, "Instanced", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        bool mSkinned{};
        ReflectProperty(mSkinned, "Skinned", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        Core::Uint64 mRevision{};
        ReflectProperty(mRevision, "Revision", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
        ReflectProperty(mVertexShader, "VertexShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
        ReflectProperty(mPixelShader, "PixelShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
        ReflectProperty(mInputLayout, "InputLayout", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
