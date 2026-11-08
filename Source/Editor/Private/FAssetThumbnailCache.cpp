/**
 * @file FAssetThumbnailCache.cpp
 * @brief 구형 재질과 정적 메시 썸네일의 GPU 캐시 구현입니다.
 * @ingroup Editor
 */

#include "Editor/FAssetThumbnailCache.h"
#include "FBonePreview.h"
#include "Core/FilePath.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Asset/FAssetManager.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/PrimitiveMesh.h"
#include "Asset/UStaticMesh.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"
#include "Asset/UMaterial.h"
#include "Asset/UTexture2D.h"
#include "RenderCore/IRendererModule.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace Editor {

    /**
     * @brief 에셋 관리자와 렌더러를 연결합니다.
     */
    Editor::FAssetThumbnailCache::FAssetThumbnailCache(Asset::FAssetManager* Assets, RenderCore::IRendererModule* Renderer)
		: mAssets{Assets},
		  mRenderer{Renderer},
		  mSphere{CoreUObject::NewObject<Asset::UStaticMesh>()},
		  mCube{CoreUObject::NewObject<Asset::UStaticMesh>()} {
        mSphere->SetMeshData(Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Sphere));
        mCube->SetMeshData(Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Cube));
    }

    /**
     * @brief 장면과 썸네일 GPU 자원을 해제합니다.
     */
    Editor::FAssetThumbnailCache::~FAssetThumbnailCache() = default;

    /**
     * @brief UI 프레임 시작 시 생성 예산과 오래된 캐시를 갱신합니다.
     */
    void Editor::FAssetThumbnailCache::BeginFrame(Core::Uint32 RenderBudget) {
        ++mFrame;
        mRenderBudget = RenderBudget;

        while (mThumbnails.size() >= 128) {
            const auto Oldest{std::min_element(mThumbnails.begin(), mThumbnails.end(), [](const auto& First, const auto& Second) {
                return First.second.mLastUsed < Second.second.mLastUsed;
            })};

            if (Oldest == mThumbnails.end() || Oldest->second.mLastUsed + 1 >= mFrame) {
                break;
            }

            mThumbnails.erase(Oldest);
        }
    }

    /**
     * @brief 타깃 주소를 유지하면서 다음 조회에서 썸네일을 갱신하도록 표시합니다.
     */
    void Editor::FAssetThumbnailCache::Invalidate() {
        ++mGeneration;
    }

    /**
     * @brief 재질 설정의 변경을 감지할 서명을 만듭니다.
     */
    std::size_t Editor::FAssetThumbnailCache::MaterialSignature(const Asset::UMaterial* Material) {
        if (Material == nullptr) {
            return 0;
        }

        std::size_t Hash{std::hash<Core::FString>{}(Material->GetShaderPath())};

        Hash ^= std::hash<Core::FString>{}(Material->GetBaseColorTexturePath()) + 0x9E3779B9 + (Hash << 6) + (Hash >> 2);
        Hash ^= std::hash<Core::FString>{}(Material->GetNormalMapTexturePath()) + 0x9E3779B9 + (Hash << 6) + (Hash >> 2);

        for (const float Value : {Material->GetSpecularStrength(), Material->GetShininess(), Material->GetNormalStrength()}) {
            Hash ^= std::bit_cast<Core::Uint32>(Value) + 0x9E3779B9 + (Hash << 6) + (Hash >> 2);
        }

        for (float Value : Material->GetBaseColor().ToArray()) {
            Hash ^= std::bit_cast<Core::Uint32>(Value) + 0x9E3779B9 + (Hash << 6) + (Hash >> 2);
        }

        return Hash ^ static_cast<std::size_t>(Material->IsTwoSided());
    }

    /**
     * @brief 메시·재질·텍스처의 표시 자원을 조회합니다.
     */
    void* Editor::FAssetThumbnailCache::GetThumbnail(const std::filesystem::path& Path, Asset::EAssetType Type) {
        if (mAssets == nullptr || mRenderer == nullptr) {
            return nullptr;
        }

        Core::FString Error{};

        if (Type == Asset::EAssetType::Texture) {
            Asset::UTexture2D* Texture{mAssets->LoadTexture(Path, Error)};

            return Texture != nullptr && Texture->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice())) ? Texture->GetShaderResourceView() : nullptr;
        }

        const Core::FString Key{Path.empty() ? Type == Asset::EAssetType::Mesh ? "@DefaultMesh" : "@DefaultMaterial" : Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(mAssets->GetRelativePath(Path, Error))))};

        if (Type == Asset::EAssetType::Material) {
            Asset::UMaterial* Material{Path.empty() ? nullptr : mAssets->LoadMaterial(Path, Error)};

            return !Path.empty() && Material == nullptr ? nullptr : Render(Key, mSphere.get(), Material, MaterialSignature(Material) ^ mGeneration);
        }

        if (Type == Asset::EAssetType::Skeleton) {
            const Asset::USkeleton* Skeleton{Path.empty() ? nullptr : mAssets->LoadSkeleton(Path, Error)};

            return Skeleton != nullptr ? Render(Key, nullptr, nullptr, mGeneration ^ static_cast<std::size_t>(Skeleton->GetRevision().GetValue()), Skeleton) : nullptr;
        }

        if (Type != Asset::EAssetType::Mesh && Type != Asset::EAssetType::SkeletalMesh) {
            return nullptr;
        }

        // 썸네일은 바인드 포즈를 표시하므로 동일한 공유 형상으로 그릴 수 있습니다.
        Asset::USkeletalMesh* Skeletal{Type == Asset::EAssetType::SkeletalMesh && !Path.empty() ? mAssets->LoadSkeletalMesh(Path, Error) : nullptr};
        Asset::UStaticMesh* Mesh{Skeletal != nullptr ? Skeletal->GetGeometry() : Type == Asset::EAssetType::SkeletalMesh ? nullptr : Path.empty() ? mCube.get() : mAssets->LoadMesh(Path, Error)};

        if (Mesh == nullptr) {
            return nullptr;
        }

        std::size_t Signature{mGeneration ^ static_cast<std::size_t>(Mesh->GetRevision().GetValue())};

        for (const Asset::FMeshMaterialSlot& Slot : Mesh->GetMeshData().GetMaterialSlots()) {
            Asset::UMaterial* Material{Slot.GetMaterialPath().empty() ? nullptr : mAssets->LoadMaterial(Core::FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};
            const std::size_t Value{MaterialSignature(Material)};

            Signature ^= Value + 0x9E3779B9 + (Signature << 6) + (Signature >> 2);
        }

        return Render(Key, Mesh, nullptr, Signature);
    }

    /**
     * @brief 편집 중인 재질을 구에 적용한 미리보기를 조회합니다.
     */
    void* Editor::FAssetThumbnailCache::GetMaterialPreview(Asset::UMaterial* Material) {
        return mRenderer != nullptr ? Render("@MaterialPreview", mSphere.get(), Material, MaterialSignature(Material) ^ mGeneration) : nullptr;
    }

    /**
     * @brief 메시 또는 구형 재질을 캐시에 렌더링합니다.
     */
    void* Editor::FAssetThumbnailCache::Render(const Core::FString& Key, Asset::UStaticMesh* Mesh, Asset::UMaterial* Material, std::size_t Signature, const Asset::USkeleton* Skeleton) {
        FThumbnail& Thumbnail{mThumbnails[Key]};
        Thumbnail.mLastUsed = mFrame;

        if (Thumbnail.mTarget != nullptr && Thumbnail.mSignature == Signature) {
            return Thumbnail.mTarget->GetNativeTexture();
        }

        if (mRenderBudget == 0) {
            return Thumbnail.mTarget != nullptr ? Thumbnail.mTarget->GetNativeTexture() : nullptr;
        }

        --mRenderBudget;

        if (mScene == nullptr) {
            mScene = mRenderer->AllocateScene();

            if (mScene != nullptr) {
                Engine::FLightSceneProxy Ambient{};
                Ambient.mIntensity = 0.25f;

                mScene->AddLight(Ambient);

                Engine::FLightSceneProxy Directional{};
                Directional.mType = Engine::ELightType::Directional;
                Directional.mDirection = Core::FVector{0.35f, 0.6f, -1.0f}.Normalized();
                Directional.mIntensity = 0.75f;

                mScene->AddLight(Directional);
            }
        }

        if (mScene == nullptr || (Mesh != nullptr && !Mesh->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice())))) {
            return nullptr;
        }

        if (Thumbnail.mTarget == nullptr) {
            Thumbnail.mTarget = mRenderer->CreateRenderTarget(256, 256);
        }

        if (Thumbnail.mTarget == nullptr) {
            return nullptr;
        }

        Core::FBox Bounds{};

        if (Skeleton != nullptr) {
            mScene->RemovePrimitive(mPrimitiveId);

            const auto& Transforms{Skeleton->GetGlobalBindPoses()};
            const Core::FVector FirstJoint{Transforms.front().TransformPosition({})};

            Bounds = Core::FBox{FirstJoint, FirstJoint};

            for (const Core::FMatrix& Transform : Transforms) {
                const Core::FVector Joint{Transform.TransformPosition({})};

                Bounds.mMinimum = Bounds.mMinimum.Min(Joint);
                Bounds.mMaximum = Bounds.mMaximum.Max(Joint);
            }
        } else {
            Engine::FPrimitiveSceneProxy Proxy{};
            Proxy.mId = mPrimitiveId;
            Proxy.mMesh = Mesh;
            Proxy.mBounds = Mesh->GetBounds();
            Proxy.mMaterial = Material;

            Core::FString Error{};

            for (const Asset::FMeshMaterialSlot& Slot : Mesh->GetMeshData().GetMaterialSlots()) {
                Proxy.mMaterials.push_back(mAssets != nullptr && !Slot.GetMaterialPath().empty() ? mAssets->LoadMaterial(Core::FilePath::FromUtf8(Slot.GetMaterialPath()), Error) : nullptr);
            }

            Bounds = Proxy.mBounds;
            mScene->AddPrimitive(Proxy);
        }

        const Core::FVector Center{(Bounds.mMinimum + Bounds.mMaximum) * 0.5f};
        const float Radius{std::max(Bounds.mMinimum.Distance(Bounds.mMaximum) * 0.5f, 0.01f)};

        if (!std::isfinite(Radius)) {
            return nullptr;
        }

        const Core::FQuat Rotation{Core::FRotator{-25.0f, 45.0f, 0.0f}.Quaternion()};
        Engine::FSceneView View{};
        View.mSize = {256.0f, 256.0f};
        View.mFieldOfView = 40.0f;
        View.mForward = Rotation.ForwardVector();
        View.mLocation = Center - View.mForward * Radius * 3.3f;
        View.mView = Core::FMatrix::LookTo(View.mLocation, View.mForward, Rotation.UpVector());
        View.mProjection = Core::FMatrix::Perspective(40.0f, 1.0f, std::max(Radius * 0.001f, 0.00001f), Radius * 10.0f);
        View.mViewProjection = View.mView * View.mProjection;

        Engine::FSceneViewFamily Family{mScene.get(), Thumbnail.mTarget.get(), View};
        Family.mShowFlags.mSky = false;

        Engine::FPrimitiveDrawInterface Bones{};

        if (Skeleton != nullptr) {
            for (const Editor::FBoneShape& Shape : Editor::BuildBoneShapes(Skeleton->GetBones(), Skeleton->GetGlobalBindPoses(), Radius)) {
                const Core::FVector4 Color{Shape.mMesh == Engine::EEditorMesh::Sphere ? Core::FVector4{0.88f, 0.92f, 1.0f, 1.0f} : Core::FVector4{0.49f, 0.7f, 0.9f, 1.0f}};

                Bones.DrawMesh({Shape.mMesh, Shape.mWorld, Color});
            }

            Family.mBackgroundOverlay = &Bones;
        }

        mRenderer->BeginRenderingViewFamily(Family);

        Thumbnail.mSignature = Signature;

        return Thumbnail.mTarget->GetNativeTexture();
    }

}
