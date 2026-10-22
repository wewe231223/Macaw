#pragma once

#include "Core/ReflectionMacros.h"
#include "Renderer/FMeshResource.h"

struct FSceneDrawContext {
public:
    ReflectTypes(FSceneDrawContext);

public:
    ReflectStruct(FSceneDrawContext);

private:
    ReflectMembers();

public:
    ID3D11DeviceContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    const std::array<FMeshResource, 3>* mMeshes{};
    ReflectProperty(mMeshes, "Meshes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    ID3D11VertexShader* mVertexShader{};
    ReflectProperty(mVertexShader, "VertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    ID3D11PixelShader* mPixelShader{};
    ReflectProperty(mPixelShader, "PixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    ID3D11InputLayout* mInputLayout{};
    ReflectProperty(mInputLayout, "InputLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    ID3D11Buffer* mViewBuffer{};
    ReflectProperty(mViewBuffer, "ViewBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
