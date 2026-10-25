#include "Engine/UStaticMesh.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

using namespace Microsoft::WRL;

UStaticMesh::UStaticMesh() = default;

UStaticMesh::~UStaticMesh() = default;

bool UStaticMesh::SetMeshData(FMeshData MeshData) {
    const std::size_t VertexCount{MeshData.GetPositions().size()};
    const std::size_t ChannelCount{std::max<std::size_t>(1, MeshData.GetUVChannels().size())};
    constexpr std::size_t MaximumBytes{std::numeric_limits<Uint32>::max()};

    if (VertexCount == 0 || MeshData.GetIndices().empty() || MeshData.GetIndices().size() % 3 != 0 || VertexCount > MaximumBytes / GetTangentStride() || MeshData.GetIndices().size() > MaximumBytes / sizeof(Uint32) || ChannelCount > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES / (sizeof(float) * 2) || VertexCount > MaximumBytes / (ChannelCount * sizeof(float) * 2)) {
        return false;
    }

    if ((!MeshData.GetNormals().empty() && MeshData.GetNormals().size() != VertexCount) || (!MeshData.GetTangents().empty() && MeshData.GetTangents().size() != VertexCount) || (!MeshData.GetColors().empty() && MeshData.GetColors().size() != VertexCount)) {
        return false;
    }

    for (Uint32 Index : MeshData.GetIndices()) {
        if (Index >= VertexCount) {
            return false;
        }
    }

    for (const FVector& Normal : MeshData.GetNormals()) {
        if (!Normal.IsFinite()) {
            return false;
        }
    }

    for (const FVector4& Tangent : MeshData.GetTangents()) {
        if (!Tangent.IsFinite()) {
            return false;
        }
    }

    for (const FVector4& Color : MeshData.GetColors()) {
        if (!Color.IsFinite()) {
            return false;
        }
    }

    for (const TArray<FVector2>& Channel : MeshData.GetUVChannels()) {
        if (Channel.size() != VertexCount) {
            return false;
        }

        for (const FVector2& UV : Channel) {
            if (!UV.IsFinite()) {
                return false;
            }
        }
    }

    FVector Minimum{MeshData.GetPositions().front()};
    FVector Maximum{Minimum};

    for (const FVector& Position : MeshData.GetPositions()) {
        if (!Position.IsFinite()) {
            return false;
        }

        Minimum = Minimum.Min(Position);
        Maximum = Maximum.Max(Position);
    }

    MeshData.SetBounds({Minimum, Maximum});
    mMeshData = std::move(MeshData);
    ReleaseResources();
    return true;
}

const FMeshData& UStaticMesh::GetMeshData() const {
    return mMeshData;
}

const FBox& UStaticMesh::GetBounds() const {
    return mMeshData.GetBounds();
}

Uint32 UStaticMesh::GetVertexCount() const {
    return static_cast<Uint32>(mMeshData.GetPositions().size());
}

Uint32 UStaticMesh::GetIndexCount() const {
    return static_cast<Uint32>(mMeshData.GetIndices().size());
}

Uint32 UStaticMesh::GetUVChannelCount() const {
    return static_cast<Uint32>(mMeshData.GetUVChannels().size());
}

bool UStaticMesh::IsValid() const {
    return !mMeshData.GetPositions().empty() && !mMeshData.GetIndices().empty();
}

bool UStaticMesh::CreateResources(ID3D11Device* Device) {
    if (Device == nullptr || !IsValid()) {
        return false;
    }

    if (HasResources()) {
        ComPtr<ID3D11Device> ResourceDevice{};
        mPositionBuffer->GetDevice(ResourceDevice.GetAddressOf());

        if (ResourceDevice.Get() == Device) {
            return true;
        }
    }

    const std::size_t VertexCount{mMeshData.GetPositions().size()};
    const std::size_t ChannelCount{std::max<std::size_t>(1, GetUVChannelCount())};
    TArray<std::array<float, 3>> Positions{};
    TArray<std::array<float, 7>> Tangents{};
    TArray<std::array<float, 2>> UVs{};
    TArray<std::array<float, 4>> Colors{};
    Positions.reserve(VertexCount);
    Tangents.reserve(VertexCount);
    UVs.reserve(VertexCount * ChannelCount);
    Colors.reserve(VertexCount);

    for (std::size_t Index{}; Index < VertexCount; ++Index) {
        Positions.push_back(mMeshData.GetPositions()[Index].ToArray());
        const FVector Normal{mMeshData.GetNormals().empty() ? FVector{0.0f, 0.0f, 1.0f} : mMeshData.GetNormals()[Index]};
        const FVector Axis{std::abs(Normal.z) < 0.99f ? FVector{0.0f, 0.0f, 1.0f} : FVector{0.0f, 1.0f, 0.0f}};
        const FVector DefaultTangent{Axis.Cross(Normal).Normalized()};
        const FVector4 Tangent{mMeshData.GetTangents().empty() ? FVector4{DefaultTangent, 1.0f} : mMeshData.GetTangents()[Index]};
        Tangents.push_back({Normal.x, Normal.y, Normal.z, Tangent.x, Tangent.y, Tangent.z, Tangent.w});
        const FVector4 Color{mMeshData.GetColors().empty() ? FVector4{1.0f, 1.0f, 1.0f, 1.0f} : mMeshData.GetColors()[Index]};
        Colors.push_back({Color.x, Color.y, Color.z, Color.w});

        for (std::size_t Channel{}; Channel < ChannelCount; ++Channel) {
            const FVector2 UV{GetUVChannelCount() == 0 ? FVector2{} : mMeshData.GetUVChannels()[Channel][Index]};
            UVs.push_back({UV.x, UV.y});
        }
    }

    std::array<ComPtr<ID3D11Buffer>, 5> Buffers{};

    if (!CreateBuffer(Device, Positions.data(), Positions.size() * GetPositionStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[0]) || !CreateBuffer(Device, Tangents.data(), Tangents.size() * GetTangentStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[1]) || !CreateBuffer(Device, UVs.data(), UVs.size() * sizeof(std::array<float, 2>), D3D11_BIND_VERTEX_BUFFER, Buffers[2]) || !CreateBuffer(Device, Colors.data(), Colors.size() * GetColorStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[3]) || !CreateBuffer(Device, mMeshData.GetIndices().data(), mMeshData.GetIndices().size() * sizeof(Uint32), D3D11_BIND_INDEX_BUFFER, Buffers[4])) {
        return false;
    }

    mPositionBuffer = std::move(Buffers[0]);
    mTangentBuffer = std::move(Buffers[1]);
    mUVBuffer = std::move(Buffers[2]);
    mColorBuffer = std::move(Buffers[3]);
    mIndexBuffer = std::move(Buffers[4]);
    mResourceBytes = VertexCount * (static_cast<std::size_t>(GetPositionStride()) + GetTangentStride() + GetUVStride() + GetColorStride()) + mMeshData.GetIndices().size() * sizeof(Uint32);
    return true;
}

void UStaticMesh::ReleaseResources() {
    mPositionBuffer.Reset();
    mTangentBuffer.Reset();
    mUVBuffer.Reset();
    mColorBuffer.Reset();
    mIndexBuffer.Reset();
    mResourceBytes = 0;
}

bool UStaticMesh::HasResources() const {
    return mPositionBuffer != nullptr && mTangentBuffer != nullptr && mUVBuffer != nullptr && mColorBuffer != nullptr && mIndexBuffer != nullptr;
}

ID3D11Buffer* UStaticMesh::GetPositionBuffer() const {
    return mPositionBuffer.Get();
}

ID3D11Buffer* UStaticMesh::GetTangentBuffer() const {
    return mTangentBuffer.Get();
}

ID3D11Buffer* UStaticMesh::GetUVBuffer() const {
    return mUVBuffer.Get();
}

ID3D11Buffer* UStaticMesh::GetColorBuffer() const {
    return mColorBuffer.Get();
}

ID3D11Buffer* UStaticMesh::GetIndexBuffer() const {
    return mIndexBuffer.Get();
}

Uint32 UStaticMesh::GetUVStride() const {
    return static_cast<Uint32>(std::max<Uint32>(1, GetUVChannelCount()) * sizeof(float) * 2);
}

std::size_t UStaticMesh::GetResourceBytes() const {
    return mResourceBytes;
}

Uint32 UStaticMesh::GetPositionStride() {
    return sizeof(std::array<float, 3>);
}

Uint32 UStaticMesh::GetTangentStride() {
    return sizeof(std::array<float, 7>);
}

Uint32 UStaticMesh::GetColorStride() {
    return sizeof(std::array<float, 4>);
}

bool UStaticMesh::CreateBuffer(ID3D11Device* Device, const void* Data, std::size_t Bytes, Uint32 BindFlags, ComPtr<ID3D11Buffer>& Buffer) {
    if (Device == nullptr || Data == nullptr || Bytes == 0 || Bytes > std::numeric_limits<UINT>::max()) {
        return false;
    }

    D3D11_BUFFER_DESC Description{};
    Description.ByteWidth = static_cast<UINT>(Bytes);
    Description.Usage = D3D11_USAGE_IMMUTABLE;
    Description.BindFlags = BindFlags;
    D3D11_SUBRESOURCE_DATA InitialData{};
    InitialData.pSysMem = Data;
    return SUCCEEDED(Device->CreateBuffer(&Description, &InitialData, Buffer.GetAddressOf()));
}
