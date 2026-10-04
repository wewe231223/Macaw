#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "RenderCore/FVertexAttribute.h"

class UMesh;

class FMeshRenderResource {
private:
    struct FLODBuffers {
        std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, static_cast<std::size_t>(EVertexAttribute::MAX)> mVertices{};

        Microsoft::WRL::ComPtr<ID3D11Buffer> mIndices{};
    };

public:
    FMeshRenderResource() = default;
    ~FMeshRenderResource() = default;

    FMeshRenderResource(const FMeshRenderResource&) = delete;
    FMeshRenderResource& operator=(const FMeshRenderResource&) = delete;
    FMeshRenderResource(FMeshRenderResource&&) noexcept = default;
    FMeshRenderResource& operator=(FMeshRenderResource&&) noexcept = default;

public:
    bool Initialize(ID3D11Device* Device, const UMesh& Mesh);
    ID3D11Buffer* GetVertexBuffer(EVertexAttribute Attribute, Uint32 Level = 0) const;
    ID3D11Buffer* GetIndexBuffer(Uint32 Level = 0) const;

private:
    static bool CreateBuffer(ID3D11Device* Device, const void* Data, std::size_t ByteSize, UINT BindFlags, Microsoft::WRL::ComPtr<ID3D11Buffer>& Buffer);

private:
    TArray<FLODBuffers> mLODs{};
};
