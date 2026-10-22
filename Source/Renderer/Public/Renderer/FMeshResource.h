#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <d3d11.h>
#include <wrl/client.h>

struct FMeshResource {
public:
    ReflectTypes(FMeshResource);

public:
    ReflectStruct(FMeshResource);

private:
    ReflectMembers();

public:
    Microsoft::WRL::ComPtr<ID3D11Buffer> mVertices{};
    ReflectProperty(mVertices, "Vertices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11Buffer> mIndices{};
    ReflectProperty(mIndices, "Indices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Uint32 mIndexCount{};
    ReflectProperty(mIndexCount, "IndexCount", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::size_t mBytes{};
    ReflectProperty(mBytes, "Bytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
