#include "pch.h"
#include "Render/FMeshRenderResource.h"
#include "Asset/UMesh.h"
#include <cstring>

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
        const TArray<Uint32>& Indices{Mesh.GetIndices(static_cast<int>(Level))};

        if (Indices.empty() || Indices.size() > UINT32_MAX) {
            return false;
        }

        for (std::size_t Index{}; Index < LOD.mVertices.size(); ++Index) {
            const EVertexAttribute Attribute{static_cast<EVertexAttribute>(Index)};
            const void* Data{Mesh.GetVertexData(Attribute, static_cast<int>(Level))};
            const Uint32 Count{Mesh.GetVertexAttributeCount(Attribute, static_cast<int>(Level))};
            const Uint32 Stride{Mesh.GetVertexStride(Attribute)};

            if (Data == nullptr || Count == 0 || Stride == 0) {
                continue;
            }

            if (Level > 0 && Data == Mesh.GetVertexData(Attribute) && Indices.data() == Mesh.GetIndices().data()) {
                LOD.mVertices[Index] = Buffers.front().mVertices[Index];
                continue;
            }

            if (Indices.size() > UINT32_MAX / Stride) {
                return false;
            }

            TArray<Uint8> Vertices{};

            Vertices.resize(Indices.size() * Stride);

            for (std::size_t VertexIndex{}; VertexIndex < Indices.size(); ++VertexIndex) {
                const Uint32 SourceIndex{Indices[VertexIndex]};

                if (SourceIndex >= Count) {
                    return false;
                }

                std::memcpy(Vertices.data() + VertexIndex * Stride, static_cast<const Uint8*>(Data) + static_cast<std::size_t>(SourceIndex) * Stride, Stride);
            }

            if (!CreateBuffer(Device, Vertices.data(), Vertices.size(), LOD.mVertices[Index])) {
                return false;
            }
        }

        LOD.mVertexCount = static_cast<Uint32>(Indices.size());
    }

    mLODs = std::move(Buffers);

    return true;
}

bool FMeshRenderResource::CreateBuffer(ID3D11Device* Device, const void* Data, std::size_t ByteSize, Microsoft::WRL::ComPtr<ID3D11Buffer>& Buffer) {
    if (Data == nullptr || ByteSize == 0 || ByteSize > UINT32_MAX) {
        return false;
    }

    D3D11_BUFFER_DESC Description{};

    Description.ByteWidth = static_cast<UINT>(ByteSize);
    Description.Usage = D3D11_USAGE_IMMUTABLE;
    Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA InitialData{};

    InitialData.pSysMem = Data;

    return SUCCEEDED(Device->CreateBuffer(&Description, &InitialData, Buffer.GetAddressOf()));
}

ID3D11Buffer* FMeshRenderResource::GetVertexBuffer(EVertexAttribute Attribute, Uint32 Level) const {
    const std::size_t Index{static_cast<std::size_t>(Attribute)};

    if (mLODs.empty() || Index >= mLODs.front().mVertices.size()) {
        return nullptr;
    }

    const FLODBuffers& LOD{Level < mLODs.size() && mLODs[Level].mVertexCount != 0 ? mLODs[Level] : mLODs.front()};

    return LOD.mVertices[Index].Get();
}
