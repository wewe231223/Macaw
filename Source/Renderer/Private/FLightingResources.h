#pragma once

#include "FTiledLightGrid.h"

#include <d3d11.h>
#include <wrl/client.h>

struct FLightingConstants {
public:
    ReflectTypes(FLightingConstants);

public:
    ReflectStruct(FLightingConstants);

private:
    ReflectMembers();

public:
    FVector4 mCameraAndCount{};
    ReflectProperty(mCameraAndCount, "CameraAndCount", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<Uint32, 4> mTileDimensions{};
    ReflectProperty(mTileDimensions, "TileDimensions", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FLightingResources {
public:
    ReflectTypes(FLightingResources);

public:
    ReflectStruct(FLightingResources);

public:
    bool Upload(ID3D11Device* Device, ID3D11DeviceContext* Context, std::span<const FLightSceneProxy> Lights, const FSceneView& View);
    void Bind(ID3D11DeviceContext* Context) const;
    void Unbind(ID3D11DeviceContext* Context) const;

private:
    ReflectMembers();

private:
    bool UploadBuffer(ID3D11Device* Device, ID3D11DeviceContext* Context, Uint32 Index, const void* Data, Uint32 Count, Uint32 Stride);

private:
    FTiledLightGrid mGrid{};
    ReflectProperty(mGrid, "Grid", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, 3> mBuffers{};
    ReflectProperty(mBuffers, "Buffers", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, 3> mViews{};
    ReflectProperty(mViews, "Views", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<Uint32, 3> mCapacities{};
    ReflectProperty(mCapacities, "Capacities", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
    ReflectProperty(mConstants, "Constants", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
