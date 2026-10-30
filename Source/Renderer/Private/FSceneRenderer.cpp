/**
 * @file FSceneRenderer.cpp
 * @brief 뷰 프러스텀으로 프리미티브를 선별하여 장면 드로 호출을 제출합니다.
 * @ingroup Renderer
 */

#include "FSceneRenderer.h"
#include "RendererStats.h"
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
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <functional>
#include <bit>
#include <limits>
#include <span>

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
    static FStat MeshPass{"render", "Opaque mesh pass CPU (inclusive)", EStatUnit::Milliseconds};
    const FScopedStatTimer StatTimer{MeshPass};
    ID3D11DeviceContext* Context{mDrawContext.mContext};
    FRenderer* Renderer{mDrawContext.mRenderer};
    Microsoft::WRL::ComPtr<ID3D11Device> Device{};
    Context->GetDevice(Device.GetAddressOf());
    Context->UpdateSubresource(mDrawContext.mViewBuffer, 0, nullptr, &mFamily->mView.mViewProjection, 0, 0);
    ID3D11Buffer* ViewBuffer{mDrawContext.mViewBuffer};
    Context->VSSetConstantBuffers(1, 1, &ViewBuffer);
    Context->PSSetConstantBuffers(1, 1, &ViewBuffer);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    if (!Renderer->PrepareLighting(*mFamily)) {
        return;
    }

    TArray<FMeshDrawCommand> Commands{};
    Commands.reserve(mScene->GetPrimitives().size());
    UMaterial* DefaultMaterial{Renderer->GetDefaultMaterial()};
    FMaterialPipeline* DefaultPipeline{Renderer->GetMaterialPipeline(*DefaultMaterial, mFamily->mViewMode)};
    if (DefaultPipeline == nullptr || !DefaultMaterial->CreateResources(Device.Get(), Context)) {
        Renderer->FinishLighting();
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
            FMaterialPipeline* Pipeline{Renderer->GetMaterialPipeline(*Material, mFamily->mViewMode)};
            Commands.emplace_back(Pipeline != nullptr ? Pipeline : DefaultPipeline, Material, Mesh, &Primitive, SubMesh.GetFirstIndex(), SubMesh.GetIndexCount());
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
        if (First.mMesh != Second.mMesh) {
            return Less(First.mMesh, Second.mMesh);
        }
        if (First.mFirstIndex != Second.mFirstIndex) {
            return First.mFirstIndex < Second.mFirstIndex;
        }
        return First.mIndexCount < Second.mIndexCount;
    });

    if (std::any_of(Commands.begin(), Commands.end(), [](const FMeshDrawCommand& Command) {
        return Command.mPipeline->IsInstanced();
    })) {
        constexpr Uint32 MaximumCapacity{std::numeric_limits<Uint32>::max() / sizeof(FObjectConstants)};
        if (Commands.size() > MaximumCapacity) {
            ErrorHandler::Fatal("Renderer", "Mesh instance buffer capacity exceeded");
        }
        TArray<FObjectConstants> Instances{};
        Instances.reserve(Commands.size());
        for (const FMeshDrawCommand& Command : Commands) {
            Instances.push_back(Command.mPrimitive->mObjectConstants);
        }
        const Uint32 Count{static_cast<Uint32>(Instances.size())};
        const Uint32 Capacity{std::min(std::bit_ceil(Count), MaximumCapacity)};
        FString Error{};
        if (!mDrawContext.mInstanceBuffer->Upload(Context, std::as_bytes(std::span<const FObjectConstants>{Instances}), sizeof(FObjectConstants), Capacity, Error)) {
            ErrorHandler::Fatal("Renderer", Error);
        }
        ID3D11Buffer* InstanceBuffer{mDrawContext.mInstanceBuffer->GetBuffer()};
        const UINT Stride{sizeof(FObjectConstants)};
        const UINT Offset{};
        Context->IASetVertexBuffers(4, 1, &InstanceBuffer, &Stride, &Offset);
    }

    FMaterialPipeline* LastPipeline{};
    UMaterial* LastMaterial{};
    UStaticMesh* LastMesh{};
    const std::array<UINT, 4> Offsets{};
    for (std::size_t First{}; First < Commands.size();) {
        const FMeshDrawCommand& Command{Commands[First]};
        std::size_t End{First + 1};
        if (Command.mPipeline->IsInstanced()) {
            while (End < Commands.size() && Commands[End].mPipeline == Command.mPipeline && Commands[End].mMaterial == Command.mMaterial && Commands[End].mMesh == Command.mMesh && Commands[End].mFirstIndex == Command.mFirstIndex && Commands[End].mIndexCount == Command.mIndexCount) {
                ++End;
            }
        }
        const Uint32 InstanceCount{static_cast<Uint32>(End - First)};
        if (LastPipeline != Command.mPipeline) {
            RendererStats::PipelineBinds.Add();
            Command.mPipeline->Bind(Context);
            if (mFamily->mViewMode == EViewMode::Wireframe) {
                Context->RSSetState(Renderer->GetWireframeRasterizer());
            }
            LastPipeline = Command.mPipeline;
            LastMaterial = nullptr;
        }
        if (LastMaterial != Command.mMaterial) {
            RendererStats::MaterialBinds.Add();
            ID3D11Buffer* Constants{Command.mMaterial->GetConstantBuffer()};
            UTexture2D* Texture{Command.mMaterial->GetBaseColorTexture()};
            ID3D11ShaderResourceView* Resource{Texture != nullptr ? Texture->GetShaderResourceView() : Renderer->GetDefaultMaterialTexture()};
            ID3D11SamplerState* Sampler{Renderer->GetMaterialSampler()};
            Context->VSSetConstantBuffers(2, 1, &Constants);
            Context->PSSetConstantBuffers(2, 1, &Constants);
            UTexture2D* NormalTexture{Command.mMaterial->GetNormalMapTexture()};
            ID3D11ShaderResourceView* NormalResource{NormalTexture != nullptr ? NormalTexture->GetShaderResourceView() : nullptr};
            Context->PSSetShaderResources(0, 1, &Resource);
            Context->VSSetShaderResources(1, 1, &NormalResource);
            Context->PSSetShaderResources(1, 1, &NormalResource);
            Context->VSSetSamplers(0, 1, &Sampler);
            Context->PSSetSamplers(0, 1, &Sampler);
            LastMaterial = Command.mMaterial;
        }
        if (LastMesh != Command.mMesh) {
            RendererStats::MeshBinds.Add();
            UStaticMesh* Mesh{Command.mMesh};
            const std::array<ID3D11Buffer*, 4> Buffers{Mesh->GetPositionBuffer(), Mesh->GetTangentBuffer(), Mesh->GetUVBuffer(), Mesh->GetColorBuffer()};
            const std::array<UINT, 4> Strides{UStaticMesh::GetPositionStride(), UStaticMesh::GetTangentStride(), Mesh->GetUVStride(), UStaticMesh::GetColorStride()};
            Context->IASetVertexBuffers(0, static_cast<UINT>(Buffers.size()), Buffers.data(), Strides.data(), Offsets.data());
            Context->IASetIndexBuffer(Mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
            LastMesh = Mesh;
        }
        if (Command.mPipeline->IsInstanced()) {
            Context->DrawIndexedInstanced(Command.mIndexCount, InstanceCount, Command.mFirstIndex, 0, static_cast<UINT>(First));
        } else {
            ID3D11Buffer* ObjectBuffer{Command.mPrimitive->mObjectBuffer.Get()};
            Context->VSSetConstantBuffers(0, 1, &ObjectBuffer);
            Context->PSSetConstantBuffers(0, 1, &ObjectBuffer);
            Context->DrawIndexed(Command.mIndexCount, Command.mFirstIndex, 0);
        }
        RendererStats::Triangles.Add(static_cast<double>(Command.mIndexCount / 3) * InstanceCount);
        ++mStats->mDrawCalls;
        First = End;
    }
    ID3D11Buffer* EmptyInstanceBuffer{};
    Context->IASetVertexBuffers(4, 1, &EmptyInstanceBuffer, Offsets.data(), Offsets.data());
    ID3D11ShaderResourceView* Empty{};
    Context->PSSetShaderResources(0, 1, &Empty);
    Context->VSSetShaderResources(1, 1, &Empty);
    Context->PSSetShaderResources(1, 1, &Empty);
    Renderer->FinishLighting();
    mStats->mTransformUpdates += mScene->TakeTransformUpdates();
}

FMeshDrawCommand::FMeshDrawCommand(FMaterialPipeline* Pipeline, UMaterial* Material, UStaticMesh* Mesh, const FScenePrimitive* Primitive, Uint32 FirstIndex, Uint32 IndexCount)
	: mPipeline{Pipeline},
	  mMaterial{Material},
	  mMesh{Mesh},
	  mPrimitive{Primitive},
	  mFirstIndex{FirstIndex},
	  mIndexCount{IndexCount} {
}
