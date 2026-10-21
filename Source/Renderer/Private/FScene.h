#pragma once

#include "Engine/FSceneInterface.h"
#include "Core/GuidHash.h"
#include "FScenePrimitive.h"

#include <d3d11.h>
#include <wrl/client.h>

class FScene final : public FSceneInterface {
public:
    FScene(ID3D11Device& Device, ID3D11DeviceContext& Context);
    ~FScene() override;

public:
    void AddPrimitive(const FPrimitiveSceneProxy& Proxy) override;
    void UpdatePrimitiveTransform(const FGuid& Id, const FMatrix& World, const FBox& Bounds) override;
    void RemovePrimitive(const FGuid& Id) override;

    std::size_t GetPrimitiveCount() const override;
    Uint64 GetRevision() const override;

    const TArray<FScenePrimitive>& GetPrimitives() const;
    Uint32 TakeTransformUpdates() const;

private:
    void Upload(FScenePrimitive& Primitive);

private:
    ID3D11Device* mDevice{};
    ID3D11DeviceContext* mContext{};

    TArray<FScenePrimitive> mPrimitives{};
    TMap<FGuid, std::size_t, FGuidHash> mIndices{};

    Uint64 mRevision{};
    mutable Uint32 mTransformUpdates{};
};
