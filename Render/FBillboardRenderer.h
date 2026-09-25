#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "FMath.h"
#include "STL.h"
#include "Core/Base/FRenderProbe.h"
#include "Core/Buffer/FGraphicsBuffer.h"
#include "Core/Buffer/TGraphicsRootConstants.h"

class FAssetRegistry;

class FBillboardRenderer {
private:
    struct FBillboardViewConstans { FMatrix mViewProjection{}; FMatrix mCameraWorld{}; };

    static_assert(sizeof(FBillboardViewConstans) == sizeof(Uint32) * 32);

public:
    FBillboardRenderer() = default;
    ~FBillboardRenderer() = default;

    bool Initialize(ID3D11Device* InDevice, std::uint32_t InitialCapacity = 256);
    void Render(ID3D11DeviceContext* Context, const TArray<FBillboardProbe>& BillboardProbe, const CameraProbe& Camera, FAssetRegistry* AssetRegistry);

private:
    bool EnsureCapacity(Uint32 RequiredCapacity);

private:
    ID3D11Device* mDevice{nullptr};

    FGraphicsBuffer mInstanceBuffer{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mInstanceBufferSrv{};
    Uint32 mInstanceCapacity{0};
    TGraphicsRootConstants<32> mViewConstantBuffer{};
};
