#include "pch.h"
#include "Render/FMeshRenderResource.h"
#include "Asset/UMesh.h"

bool FMeshRenderResource::Initialize(ID3D11Device* Device, const UMesh& Mesh) {
    if (Device == nullptr || !Mesh.HasLOD(0)) {
        return false;
    }
    TArray<FLODBuffers> Buffers{};
    Buffers.resize(Mesh.GetLODCount());
    for (Uint32 Level{}; Level < Buffers.size(); ++Level) {
        if (!Mesh.HasLOD(static_cast<int>(Level))) {
            continue;
        }
        FLODBuffers& LOD{Buffers[Level]};
        for (std::size_t Index{}; Index < LOD.mVertices.size(); ++Index) {
            const EVertexAttribute Attribute{static_cast<EVertexAttribute>(Index)};
            const void* Data{Mesh.GetVertexData(Attribute, static_cast<int>(Level))};
            const std::size_t ByteSize{static_cast<std::size_t>(Mesh.GetVertexAttributeCount(Attribute, static_cast<int>(Level))) * Mesh.GetVertexStride(Attribute)};
            if (Data == nullptr || ByteSize == 0) {
                continue;
            }
            if (Level > 0 && Data == Mesh.GetVertexData(Attribute)) {
                LOD.mVertices[Index] = Buffers.front().mVertices[Index];
            } else if (!CreateBuffer(Device, Data, ByteSize, D3D11_BIND_VERTEX_BUFFER, LOD.mVertices[Index])) {
                return false;
            }
        }
        const TArray<Uint32>& Indices{Mesh.GetIndices(static_cast<int>(Level))};
        if (Level > 0 && Indices.data() == Mesh.GetIndices().data()) {
            LOD.mIndices = Buffers.front().mIndices;
        } else if (!CreateBuffer(Device, Indices.data(), Indices.size() * sizeof(Uint32), D3D11_BIND_INDEX_BUFFER, LOD.mIndices)) {
            return false;
        }
    }
    mLODs = std::move(Buffers);
    return true;
}

bool FMeshRenderResource::CreateBuffer(ID3D11Device* Device, const void* Data, std::size_t ByteSize, UINT BindFlags, Microsoft::WRL::ComPtr<ID3D11Buffer>& Buffer) {
    if (Data == nullptr || ByteSize == 0 || ByteSize > UINT32_MAX) {
        return false;
    }
    D3D11_BUFFER_DESC Description{};
    Description.ByteWidth = static_cast<UINT>(ByteSize);
    Description.Usage = D3D11_USAGE_IMMUTABLE;
    Description.BindFlags = BindFlags;
    D3D11_SUBRESOURCE_DATA InitialData{};
    InitialData.pSysMem = Data;
    return SUCCEEDED(Device->CreateBuffer(&Description, &InitialData, Buffer.GetAddressOf()));
}

ID3D11Buffer* FMeshRenderResource::GetVertexBuffer(EVertexAttribute Attribute, Uint32 Level) const {
    const std::size_t Index{static_cast<std::size_t>(Attribute)};
    if (mLODs.empty() || Index >= mLODs.front().mVertices.size()) {
        return nullptr;
    }
    const FLODBuffers& LOD{Level < mLODs.size() && mLODs[Level].mIndices != nullptr ? mLODs[Level] : mLODs.front()};
    return LOD.mVertices[Index].Get();
}

ID3D11Buffer* FMeshRenderResource::GetIndexBuffer(Uint32 Level) const {
    if (mLODs.empty()) {
        return nullptr;
    }
    return Level < mLODs.size() && mLODs[Level].mIndices != nullptr ? mLODs[Level].mIndices.Get() : mLODs.front().mIndices.Get();
}
