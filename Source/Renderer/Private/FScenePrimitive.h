#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FPrimitiveSceneProxy.h"

#include <d3d11.h>
#include <wrl/client.h>

struct FScenePrimitive {
public:
    ReflectTypes(FScenePrimitive);

public:
    ReflectStruct(FScenePrimitive);

private:
    ReflectMembers();

public:
    FPrimitiveSceneProxy mProxy{};
    ReflectProperty(mProxy, "Proxy", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11Buffer> mObjectBuffer{};
    ReflectProperty(mObjectBuffer, "ObjectBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
