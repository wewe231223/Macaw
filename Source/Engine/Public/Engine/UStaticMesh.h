#pragma once

#include "CoreUObject/UObject.h"
#include "Engine/FMeshData.h"

#include <d3d11.h>
#include <wrl/client.h>

class UStaticMesh final : public UObject {
public:
    ReflectTypes(UStaticMesh, UObject);

public:
    UStaticMesh();
    ~UStaticMesh() override;

public:
    ReflectClass(UStaticMesh);

    bool SetMeshData(FMeshData MeshData);
    const FMeshData& GetMeshData() const;
    const FBox& GetBounds() const;
    Uint32 GetVertexCount() const;
    Uint32 GetIndexCount() const;
    Uint32 GetUVChannelCount() const;
    Uint64 GetRenderRevision() const;
    bool IsValid() const;
    bool CreateResources(ID3D11Device* Device);
    void ReleaseResources();
    bool HasResources() const;
    ID3D11Buffer* GetPositionBuffer() const;
    ID3D11Buffer* GetTangentBuffer() const;
    ID3D11Buffer* GetUVBuffer() const;
    ID3D11Buffer* GetColorBuffer() const;
    ID3D11Buffer* GetIndexBuffer() const;
    Uint32 GetUVStride() const;
    std::size_t GetResourceBytes() const;
    static Uint32 GetPositionStride();
    static Uint32 GetTangentStride();
    static Uint32 GetColorStride();

private:
    ReflectMembers();
    static bool CreateBuffer(ID3D11Device* Device, const void* Data, std::size_t Bytes, Uint32 BindFlags, Microsoft::WRL::ComPtr<ID3D11Buffer>& Buffer);

private:
    FMeshData mMeshData{};
    ReflectProperty(mMeshData, "MeshData", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mRenderRevision{1};
    ReflectProperty(mRenderRevision, "RenderRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mPositionBuffer{};
    ReflectProperty(mPositionBuffer, "PositionBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mTangentBuffer{};
    ReflectProperty(mTangentBuffer, "TangentBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mUVBuffer{};
    ReflectProperty(mUVBuffer, "UVBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mColorBuffer{};
    ReflectProperty(mColorBuffer, "ColorBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer{};
    ReflectProperty(mIndexBuffer, "IndexBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mResourceRevision{};
    ReflectProperty(mResourceRevision, "ResourceRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mResourceBytes{};
    ReflectProperty(mResourceBytes, "ResourceBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
