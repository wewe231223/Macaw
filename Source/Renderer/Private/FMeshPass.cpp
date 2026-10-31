#include "FMeshPass.h"

#include "FScene.h"
#include "FScenePrimitive.h"
#include "Asset/UStaticMesh.h"
#include "Core/Math/FBox.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"
#include "Engine/FSceneViewFamily.h"

#include <algorithm>
#include <functional>

namespace Renderer {

    namespace {
        // 경계 상자의 8개 꼭짓점이 모두 같은 클립 평면 밖에 있으면 보이지 않는 것으로 판정합니다.
        bool IsVisible(const Core::FMatrix& ViewProjection, const Core::FBox& Bounds) {
            std::array<Core::Uint32, 6> Outside{};

            for (Core::Int32 Index{}; Index < 8; ++Index) {
                const Core::FVector Point{(Index & 1) ? Bounds.mMaximum.x : Bounds.mMinimum.x, (Index & 2) ? Bounds.mMaximum.y : Bounds.mMinimum.y, (Index & 4) ? Bounds.mMaximum.z : Bounds.mMinimum.z};
                const Core::FVector4 Clip{Core::FVector4{Point, 1.0f} * ViewProjection};
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
    }

    Renderer::FMeshPass::FMeshPass(Renderer::FRenderer& Renderer, ID3D11Device* Device, ID3D11DeviceContext* Context, const std::array<std::unique_ptr<Asset::UStaticMesh>, 3>& Meshes)
		: mRenderer{&Renderer},
		  mDevice{Device},
		  mContext{Context},
		  mMeshes{&Meshes} {
    }

    Renderer::FMeshPass::~FMeshPass() = default;

    bool Renderer::FMeshPass::BuildCommands(const Renderer::FScene& Scene, const Engine::FSceneViewFamily& Family, const Core::FMatrix& ViewProjection, Core::TArray<Renderer::FMeshDrawCommand>& Commands, Core::Uint32& Primitives) {
        Commands.clear();
        Primitives = 0;

        if (!BeginCommands(Family)) {
            return false;
        }

        Commands.reserve(Scene.GetPrimitives().size());

        for (const Renderer::FScenePrimitive& Primitive : Scene.GetPrimitives()) {
            if (Primitive.mProxy.mText.has_value() || Primitive.mProxy.mSprite.has_value() || !Includes(Primitive, Family) || !IsVisible(ViewProjection, Primitive.mProxy.mBounds)) {
                continue;
            }

            Asset::UStaticMesh* Mesh{Primitive.mProxy.mMesh != nullptr ? Primitive.mProxy.mMesh : (*mMeshes)[static_cast<std::size_t>(Primitive.mProxy.mPrimitive)].get()};

            if (Mesh == nullptr || (!Mesh->HasResources() && !Mesh->CreateResources(mDevice)) || Primitive.mObjectBuffer == nullptr) {
                continue;
            }

            AddCommands(Primitive, *Mesh, Family, Commands);
            ++Primitives;
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

        return true;
    }

}
