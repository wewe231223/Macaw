/**
 * @file FSceneRenderer.cpp
 * @brief 뷰 프러스텀으로 프리미티브를 선별하여 장면 드로 호출을 제출합니다.
 * @ingroup Renderer
 */

#include "FSceneRenderer.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "Engine/FSceneViewFamily.h"
#include "RenderCore/FRenderStats.h"
#include "FScenePrimitive.h"
#include "FScene.h"
#include "Engine/UStaticMesh.h"
#include "FSceneDrawContext.h"
#include "FMaterialPipeline.h"
#include "Renderer/Renderer.h"
#include "Engine/UMaterial.h"
#include "Engine/UTexture2D.h"

#include <algorithm>
#include <functional>

/**
 * @brief FSceneRenderer의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FSceneRenderer::FSceneRenderer(const FSceneViewFamily& Family, const FScene* Scene, const FSceneDrawContext& Context, FRenderStats& Stats)
	: mFamily{&Family},
	  mScene{Scene},
	  mDrawContext{Context},
	  mStats{&Stats} {
}

/**
 * @brief 경계 상자가 뷰 프러스텀과 겹치는지 검사합니다.
 */
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

/**
 * @brief 가시 프리미티브를 선별하여 드로 호출을 제출하고 통계를 갱신합니다.
 */
void FSceneRenderer::Render() {
    ID3D11DeviceContext* Context{mDrawContext.mContext};
    FRenderer* Renderer{mDrawContext.mRenderer};
    Microsoft::WRL::ComPtr<ID3D11Device> Device{};
    Context->GetDevice(Device.GetAddressOf());
    Context->UpdateSubresource(mDrawContext.mViewBuffer, 0, nullptr, &mFamily->mView.mViewProjection, 0, 0);
    ID3D11Buffer* ViewBuffer{mDrawContext.mViewBuffer};
    Context->VSSetConstantBuffers(1, 1, &ViewBuffer);
    Context->PSSetConstantBuffers(1, 1, &ViewBuffer);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    TArray<FMeshDrawCommand> Commands{};
    Commands.reserve(mScene->GetPrimitives().size());
    UMaterial* DefaultMaterial{Renderer->GetDefaultMaterial()};
    FMaterialPipeline* DefaultPipeline{Renderer->GetMaterialPipeline(*DefaultMaterial)};
    if (DefaultPipeline == nullptr || !DefaultMaterial->CreateResources(Device.Get(), Context)) {
        return;
    }
    for (const FScenePrimitive& Primitive : mScene->GetPrimitives()) {
        if (!mFamily->mShowFlags.mPrimitives || Primitive.mProxy.mText.has_value() || Primitive.mProxy.mSprite.has_value() || !IsVisible(Primitive.mProxy.mBounds)) {
            continue;
        }
        UStaticMesh* Mesh{Primitive.mProxy.mMesh != nullptr ? Primitive.mProxy.mMesh : (*mDrawContext.mMeshes)[static_cast<std::size_t>(Primitive.mProxy.mPrimitive)].get()};
        if (Mesh == nullptr || (!Mesh->HasResources() && !Mesh->CreateResources(Device.Get())) || Primitive.mObjectBuffer == nullptr) {
            continue;
        }
        for (const FSubMesh& SubMesh : Mesh->GetMeshData().GetSubMeshes()) {
            UMaterial* SlotMaterial{SubMesh.GetMaterialIndex() < Primitive.mProxy.mMaterials.size() ? Primitive.mProxy.mMaterials[SubMesh.GetMaterialIndex()] : nullptr};
            UMaterial* Material{Primitive.mProxy.mMaterial != nullptr ? Primitive.mProxy.mMaterial : SlotMaterial != nullptr ? SlotMaterial : DefaultMaterial};
            if (!Material->CreateResources(Device.Get(), Context)) {
                Material = DefaultMaterial;
            }
            FMaterialPipeline* Pipeline{Renderer->GetMaterialPipeline(*Material)};
            Commands.emplace_back(Pipeline != nullptr ? Pipeline : DefaultPipeline, Material, Mesh, Primitive.mObjectBuffer.Get(), SubMesh.GetFirstIndex(), SubMesh.GetIndexCount());
        }
        ++mStats->mVisiblePrimitives;
    }
    std::stable_sort(Commands.begin(), Commands.end(), [](const FMeshDrawCommand& First, const FMeshDrawCommand& Second) {
        const std::less<const void*> Less{};
        if (First.mPipeline != Second.mPipeline) {
            return Less(First.mPipeline, Second.mPipeline);
        }
        if (First.mMaterial != Second.mMaterial) {
            return Less(First.mMaterial, Second.mMaterial);
        }
        return Less(First.mMesh, Second.mMesh);
    });

    FMaterialPipeline* LastPipeline{};
    UMaterial* LastMaterial{};
    UStaticMesh* LastMesh{};
    const std::array<UINT, 4> Offsets{};
    for (const FMeshDrawCommand& Command : Commands) {
        if (LastPipeline != Command.mPipeline) {
            Command.mPipeline->Bind(Context);
            if (mFamily->mViewMode == EViewMode::Wireframe) {
                Context->RSSetState(Renderer->GetWireframeRasterizer());
            }
            LastPipeline = Command.mPipeline;
            LastMaterial = nullptr;
        }
        if (LastMaterial != Command.mMaterial) {
            ID3D11Buffer* Constants{Command.mMaterial->GetConstantBuffer()};
            UTexture2D* Texture{Command.mMaterial->GetBaseColorTexture()};
            ID3D11ShaderResourceView* Resource{Texture != nullptr ? Texture->GetShaderResourceView() : Renderer->GetDefaultMaterialTexture()};
            ID3D11SamplerState* Sampler{Renderer->GetMaterialSampler()};
            Context->VSSetConstantBuffers(2, 1, &Constants);
            Context->PSSetConstantBuffers(2, 1, &Constants);
            Context->PSSetShaderResources(0, 1, &Resource);
            Context->PSSetSamplers(0, 1, &Sampler);
            LastMaterial = Command.mMaterial;
        }
        if (LastMesh != Command.mMesh) {
            UStaticMesh* Mesh{Command.mMesh};
            const std::array<ID3D11Buffer*, 4> Buffers{Mesh->GetPositionBuffer(), Mesh->GetTangentBuffer(), Mesh->GetUVBuffer(), Mesh->GetColorBuffer()};
            const std::array<UINT, 4> Strides{UStaticMesh::GetPositionStride(), UStaticMesh::GetTangentStride(), Mesh->GetUVStride(), UStaticMesh::GetColorStride()};
            Context->IASetVertexBuffers(0, static_cast<UINT>(Buffers.size()), Buffers.data(), Strides.data(), Offsets.data());
            Context->IASetIndexBuffer(Mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
            LastMesh = Mesh;
        }
        ID3D11Buffer* ObjectBuffer{Command.mObjectBuffer};
        Context->VSSetConstantBuffers(0, 1, &ObjectBuffer);
        Context->PSSetConstantBuffers(0, 1, &ObjectBuffer);
        Context->DrawIndexed(Command.mIndexCount, Command.mFirstIndex, 0);
        ++mStats->mDrawCalls;
    }
    ID3D11ShaderResourceView* Empty{};
    Context->PSSetShaderResources(0, 1, &Empty);
    mStats->mTransformUpdates += mScene->TakeTransformUpdates();
}

FMeshDrawCommand::FMeshDrawCommand(FMaterialPipeline* Pipeline, UMaterial* Material, UStaticMesh* Mesh, ID3D11Buffer* ObjectBuffer, Uint32 FirstIndex, Uint32 IndexCount)
	: mPipeline{Pipeline},
	  mMaterial{Material},
	  mMesh{Mesh},
	  mObjectBuffer{ObjectBuffer},
	  mFirstIndex{FirstIndex},
	  mIndexCount{IndexCount} {
}
