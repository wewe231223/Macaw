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

namespace Renderer {

    /**
     * @brief FSceneRenderer의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Renderer::FSceneRenderer::FSceneRenderer(const Engine::FSceneViewFamily& Family, const Renderer::FScene* Scene, const Renderer::FSceneDrawContext& Context, RenderCore::FRenderStats& Stats)
		: mFamily{&Family},
		  mScene{Scene},
		  mDrawContext{Context},
		  mStats{&Stats} {
    }

    /**
     * @brief 경계 상자가 뷰 프러스텀과 겹치는지 검사합니다.
     */
    bool Renderer::FSceneRenderer::IsVisible(const Core::FBox& Bounds) const {
        const Core::FMatrix& Matrix{mFamily->mView.mViewProjection};
        std::array<Core::Uint32, 6> Outside{};

        for (Core::Int32 Index{}; Index < 8; ++Index) {
            const Core::FVector Point{(Index & 1) ? Bounds.mMaximum.x : Bounds.mMinimum.x, (Index & 2) ? Bounds.mMaximum.y : Bounds.mMinimum.y, (Index & 4) ? Bounds.mMaximum.z : Bounds.mMinimum.z};

            const Core::FVector4 Clip{Core::FVector4{Point, 1.0f} * Matrix};

            Outside[0] += Clip.x < -Clip.w;
            Outside[1] += Clip.x > Clip.w;
            Outside[2] += Clip.y < -Clip.w;
            Outside[3] += Clip.y > Clip.w;
            Outside[4] += Clip.z < 0.0f;
            Outside[5] += Clip.z > Clip.w;
        }

        return std::none_of(Outside.begin(), Outside.end(), [](Core::Uint32 Count) {
            return Count == 8;
        });
    }

    /**
     * @brief 가시 프리미티브를 선별하여 드로 호출을 제출하고 통계를 갱신합니다.
     */
    void Renderer::FSceneRenderer::Render() {
        static Core::FStat MeshPass{"render", "Opaque mesh pass CPU (inclusive)", Core::EStatUnit::Milliseconds};
        const Core::FScopedStatTimer StatTimer{MeshPass};
        ID3D11DeviceContext* Context{mDrawContext.mContext};
        Renderer::FRenderer* Renderer{mDrawContext.mRenderer};
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

        Core::TArray<Renderer::FMeshDrawCommand> Commands{};
        Commands.reserve(mScene->GetPrimitives().size());

        Engine::UMaterial* DefaultMaterial{Renderer->GetDefaultMaterial()};
        Renderer::FMaterialPipeline* DefaultPipeline{Renderer->GetMaterialPipeline(*DefaultMaterial, mFamily->mViewMode)};

        if (DefaultPipeline == nullptr || !DefaultMaterial->CreateResources(Device.Get(), Context)) {
            Renderer->FinishLighting();
            return;
        }

        for (const Renderer::FScenePrimitive& Primitive : mScene->GetPrimitives()) {
            if (!mFamily->mShowFlags.mPrimitives || Primitive.mProxy.mText.has_value() || Primitive.mProxy.mSprite.has_value() || !IsVisible(Primitive.mProxy.mBounds)) {
                continue;
            }

            Engine::UStaticMesh* Mesh{Primitive.mProxy.mMesh != nullptr ? Primitive.mProxy.mMesh : (*mDrawContext.mMeshes)[static_cast<std::size_t>(Primitive.mProxy.mPrimitive)].get()};

            if (Mesh == nullptr || (!Mesh->HasResources() && !Mesh->CreateResources(Device.Get())) || Primitive.mObjectBuffer == nullptr) {
                continue;
            }

            for (const Engine::FSubMesh& SubMesh : Mesh->GetMeshData().GetSubMeshes()) {
                Engine::UMaterial* SlotMaterial{SubMesh.GetMaterialIndex() < Primitive.mProxy.mMaterials.size() ? Primitive.mProxy.mMaterials[SubMesh.GetMaterialIndex()] : nullptr};
                Engine::UMaterial* Material{Primitive.mProxy.mMaterial != nullptr ? Primitive.mProxy.mMaterial : SlotMaterial != nullptr ? SlotMaterial : DefaultMaterial};

                if (!Material->CreateResources(Device.Get(), Context)) {
                    Material = DefaultMaterial;
                }

                Renderer::FMaterialPipeline* Pipeline{Renderer->GetMaterialPipeline(*Material, mFamily->mViewMode)};
                Commands.emplace_back(Pipeline != nullptr ? Pipeline : DefaultPipeline, Material, Mesh, &Primitive, SubMesh.GetFirstIndex(), SubMesh.GetIndexCount());
            }

            ++mStats->mVisiblePrimitives;
        }

        std::stable_sort(Commands.begin(), Commands.end(), [](const Renderer::FMeshDrawCommand& First, const Renderer::FMeshDrawCommand& Second) {
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

        if (std::any_of(Commands.begin(), Commands.end(), [](const Renderer::FMeshDrawCommand& Command) {
            return Command.mPipeline->IsInstanced();
        })) {
            constexpr Core::Uint32 MaximumCapacity{std::numeric_limits<Core::Uint32>::max() / sizeof(Renderer::FObjectConstants)};

            if (Commands.size() > MaximumCapacity) {
                Core::ErrorHandler::Fatal("Renderer", "Mesh instance buffer capacity exceeded");
            }

            Core::TArray<Renderer::FObjectConstants> Instances{};
            Instances.reserve(Commands.size());

            for (const Renderer::FMeshDrawCommand& Command : Commands) {
                Instances.push_back(Command.mPrimitive->mObjectConstants);
            }

            const Core::Uint32 Count{static_cast<Core::Uint32>(Instances.size())};
            const Core::Uint32 Capacity{std::min(std::bit_ceil(Count), MaximumCapacity)};
            Core::FString Error{};

            if (!mDrawContext.mInstanceBuffer->Upload(Context, std::as_bytes(std::span<const Renderer::FObjectConstants>{Instances}), sizeof(Renderer::FObjectConstants), Capacity, Error)) {
                Core::ErrorHandler::Fatal("Renderer", Error);
            }

            ID3D11Buffer* InstanceBuffer{mDrawContext.mInstanceBuffer->GetBuffer()};
            const UINT Stride{sizeof(Renderer::FObjectConstants)};
            const UINT Offset{};
            Context->IASetVertexBuffers(4, 1, &InstanceBuffer, &Stride, &Offset);
        }

        Renderer::FMaterialPipeline* LastPipeline{};
        Engine::UMaterial* LastMaterial{};
        Engine::UStaticMesh* LastMesh{};
        const std::array<UINT, 4> Offsets{};

        for (std::size_t First{}; First < Commands.size();) {
            const Renderer::FMeshDrawCommand& Command{Commands[First]};
            std::size_t End{First + 1};

            if (Command.mPipeline->IsInstanced()) {
                while (End < Commands.size() && Commands[End].mPipeline == Command.mPipeline && Commands[End].mMaterial == Command.mMaterial && Commands[End].mMesh == Command.mMesh && Commands[End].mFirstIndex == Command.mFirstIndex && Commands[End].mIndexCount == Command.mIndexCount) {
                    ++End;
                }
            }

            const Core::Uint32 InstanceCount{static_cast<Core::Uint32>(End - First)};

            if (LastPipeline != Command.mPipeline) {
                Renderer::RendererStats::PipelineBinds.Add();
                Command.mPipeline->Bind(Context);

                if (mFamily->mViewMode == Engine::EViewMode::Wireframe) {
                    Context->RSSetState(Renderer->GetWireframeRasterizer());
                }

                LastPipeline = Command.mPipeline;
                LastMaterial = nullptr;
            }

            if (LastMaterial != Command.mMaterial) {
                Renderer::RendererStats::MaterialBinds.Add();

                ID3D11Buffer* Constants{Command.mMaterial->GetConstantBuffer()};
                Engine::UTexture2D* Texture{Command.mMaterial->GetBaseColorTexture()};
                ID3D11ShaderResourceView* Resource{Texture != nullptr ? Texture->GetShaderResourceView() : Renderer->GetDefaultMaterialTexture()};
                ID3D11SamplerState* Sampler{Renderer->GetMaterialSampler()};
                Context->VSSetConstantBuffers(2, 1, &Constants);
                Context->PSSetConstantBuffers(2, 1, &Constants);

                Engine::UTexture2D* NormalTexture{Command.mMaterial->GetNormalMapTexture()};
                ID3D11ShaderResourceView* NormalResource{NormalTexture != nullptr ? NormalTexture->GetShaderResourceView() : nullptr};

                Context->PSSetShaderResources(0, 1, &Resource);
                Context->VSSetShaderResources(1, 1, &NormalResource);
                Context->PSSetShaderResources(1, 1, &NormalResource);
                Context->VSSetSamplers(0, 1, &Sampler);
                Context->PSSetSamplers(0, 1, &Sampler);
                LastMaterial = Command.mMaterial;
            }

            if (LastMesh != Command.mMesh) {
                Renderer::RendererStats::MeshBinds.Add();

                Engine::UStaticMesh* Mesh{Command.mMesh};
                const std::array<ID3D11Buffer*, 4> Buffers{Mesh->GetPositionBuffer(), Mesh->GetTangentBuffer(), Mesh->GetUVBuffer(), Mesh->GetColorBuffer()};
                const std::array<UINT, 4> Strides{Engine::UStaticMesh::GetPositionStride(), Engine::UStaticMesh::GetTangentStride(), Mesh->GetUVStride(), Engine::UStaticMesh::GetColorStride()};
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

            Renderer::RendererStats::Triangles.Add(static_cast<double>(Command.mIndexCount / 3) * InstanceCount);
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

    Renderer::FMeshDrawCommand::FMeshDrawCommand(Renderer::FMaterialPipeline* Pipeline, Engine::UMaterial* Material, Engine::UStaticMesh* Mesh, const Renderer::FScenePrimitive* Primitive, Core::Uint32 FirstIndex, Core::Uint32 IndexCount)
		: mPipeline{Pipeline},
		  mMaterial{Material},
		  mMesh{Mesh},
		  mPrimitive{Primitive},
		  mFirstIndex{FirstIndex},
		  mIndexCount{IndexCount} {
    }

}
