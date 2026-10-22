#include "FSceneRenderer.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "FGpuMeshVertex.h"
#include "Engine/FSceneViewFamily.h"
#include "RenderCore/FRenderStats.h"
#include "FScenePrimitive.h"
#include "FScene.h"
#include "Renderer/FMeshResource.h"
#include "FSceneDrawContext.h"

#include <algorithm>

FSceneRenderer::FSceneRenderer(const FSceneViewFamily& Family, const FScene& Scene, const FSceneDrawContext& Context, FRenderStats& Stats)
	: mFamily{&Family},
	  mScene{&Scene},
	  mDrawContext{Context},
	  mStats{&Stats} {
}

bool FSceneRenderer::IsVisible(const FBox& Bounds) const {
    const FMatrix& Matrix{mFamily->mView.mViewProjection};
    std::array<Uint32, 6> Outside{};

    for (Int32 Index{}; Index < 8; ++Index) {
        const FVector Point{(Index & 1) ? Bounds.mMaximum.x : Bounds.mMinimum.x, (Index & 2) ? Bounds.mMaximum.y : Bounds.mMinimum.y, (Index & 4) ? Bounds.mMaximum.z : Bounds.mMinimum.z};

        const FVector4 Clip{FVector4{Point, 1.0f} * Matrix};

        Outside[0] += Clip.x < -Clip.w;
        Outside[1] += Clip.x > Clip.w;
        Outside[2] += Clip.y < -Clip.w;
        Outside[3] += Clip.y > Clip.w;
        Outside[4] += Clip.z < 0.0f;
        Outside[5] += Clip.z > Clip.w;
    }

    return std::none_of(Outside.begin(), Outside.end(), [](Uint32 Count) {
        return Count == 8;
    });
}

void FSceneRenderer::Render() {
    ID3D11DeviceContext& Context{*mDrawContext.mContext};
    Context.UpdateSubresource(mDrawContext.mViewBuffer, 0, nullptr, &mFamily->mView.mViewProjection, 0, 0);

    ID3D11Buffer* ViewBuffer{mDrawContext.mViewBuffer};
    Context.VSSetConstantBuffers(1, 1, &ViewBuffer);

    Context.IASetInputLayout(mDrawContext.mInputLayout);
    Context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Context.VSSetShader(mDrawContext.mVertexShader, nullptr, 0);
    Context.PSSetShader(mDrawContext.mPixelShader, nullptr, 0);

    const UINT Stride{sizeof(FGpuMeshVertex)};
    const UINT Offset{};

    for (const FScenePrimitive& Primitive : mScene->GetPrimitives()) {
        if (!IsVisible(Primitive.mProxy.mBounds)) {
            continue;
        }

        const FMeshResource& Mesh{(*mDrawContext.mMeshes)[static_cast<std::size_t>(Primitive.mProxy.mPrimitive)]};

        if (Mesh.mVertices == nullptr) {
            continue;
        }

        ID3D11Buffer* VertexBuffer{Mesh.mVertices.Get()};
        ID3D11Buffer* ObjectBuffer{Primitive.mObjectBuffer.Get()};

        Context.IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
        Context.IASetIndexBuffer(Mesh.mIndices.Get(), DXGI_FORMAT_R32_UINT, 0);

        Context.VSSetConstantBuffers(0, 1, &ObjectBuffer);
        Context.PSSetConstantBuffers(0, 1, &ObjectBuffer);

        Context.DrawIndexed(Mesh.mIndexCount, 0, 0);

        ++mStats->mDrawCalls;
        ++mStats->mVisiblePrimitives;
    }

    mStats->mTransformUpdates += mScene->TakeTransformUpdates();
}
