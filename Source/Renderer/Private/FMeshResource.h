#pragma once

#include "Core/Types.h"

#include <d3d11.h>
#include <wrl/client.h>

struct FMeshResource {
    Microsoft::WRL::ComPtr<ID3D11Buffer> mVertices{};
    Microsoft::WRL::ComPtr<ID3D11Buffer> mIndices{};

    Uint32 mIndexCount{};
    std::size_t mBytes{};
};
