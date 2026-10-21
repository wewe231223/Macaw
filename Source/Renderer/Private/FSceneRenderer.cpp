#include "FSceneRenderer.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "Engine/FMeshVertex.h"
#include "Engine/FSceneViewFamily.h"
#include "RenderCore/FRenderStats.h"
#include "FScenePrimitive.h"
#include "FScene.h"
#include "FMeshResource.h"
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
        const FVector Point{(Index & 1) ? Bounds.mMaximum.mX : Bounds.mMinimum.mX, (Index & 2) ? Bounds.mMaximum.mY : Bounds.mMinimum.mY, (Index & 4) ? Bounds.mMaximum.mZ : Bounds.mMinimum.mZ};

        const float X{Point.mX * Matrix.mValues[0][0] + Point.mY * Matrix.mValues[1][0] + Point.mZ * Matrix.mValues[2][0] + Matrix.mValues[3][0]};
        const float Y{Point.mX * Matrix.mValues[0][1] + Point.mY * Matrix.mValues[1][1] + Point.mZ * Matrix.mValues[2][1] + Matrix.mValues[3][1]};
        const float Z{Point.mX * Matrix.mValues[0][2] + Point.mY * Matrix.mValues[1][2] + Point.mZ * Matrix.mValues[2][2] + Matrix.mValues[3][2]};
        const float W{Point.mX * Matrix.mValues[0][3] + Point.mY * Matrix.mValues[1][3] + Point.mZ * Matrix.mValues[2][3] + Matrix.mValues[3][3]};

        Outside[0] += X < -W;
        Outside[1] += X > W;
        Outside[2] += Y < -W;
        Outside[3] += Y > W;
        Outside[4] += Z < 0.0f;
        Outside[5] += Z > W;
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

    const UINT Stride{sizeof(FMeshVertex)};
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
