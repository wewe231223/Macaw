#pragma once

#include "FMeshResource.h"

struct FSceneDrawContext {
    ID3D11DeviceContext* mContext{};
    const std::array<FMeshResource, 3>* mMeshes{};

    ID3D11VertexShader* mVertexShader{};
    ID3D11PixelShader* mPixelShader{};
    ID3D11InputLayout* mInputLayout{};

    ID3D11Buffer* mViewBuffer{};
};
