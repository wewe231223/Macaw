/**
 * @file FMeshViewerContent.cpp
 * @brief 메시·재질·스켈레톤의 미리보기 장면과 텍스처의 직접 표시를 제공합니다.
 * @ingroup Viewer
 */

#include "Viewer/FMeshViewerContent.h"

#include "Viewer/FBonePreview.h"
#include "Asset/FAssetManager.h"
#include "Asset/UStaticMesh.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"
#include "Asset/UMaterial.h"
#include "Asset/UTexture2D.h"
#include "Core/FilePath.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/PrimitiveMesh.h"
#include "Engine/EditorMesh.h"
#include "RenderCore/IRendererModule.h"

#include <algorithm>

using namespace Viewer;

FMeshViewerContent::FMeshViewerContent(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets)
	: mRenderer{Renderer},
	  mAssets{Assets},
	  mScene{Renderer, Assets} {
}

FMeshViewerContent::~FMeshViewerContent() = default;

bool FMeshViewerContent::SetAsset(CoreUObject::UObject* Object, Asset::EAssetType Type, Core::FString& Error) {
    Error.clear();

    if (mRenderer == nullptr || Object == nullptr) {
        Error = "The viewer requires a renderer and an asset";

        return false;
    }

    switch (Type) {
    case Asset::EAssetType::Texture:
        mTexture = CoreUObject::Cast<Asset::UTexture2D>(Object);
        break;
    case Asset::EAssetType::Skeleton:
        mSkeleton = CoreUObject::Cast<Asset::USkeleton>(Object);
        break;
    case Asset::EAssetType::Material:
        mMaterial = CoreUObject::Cast<Asset::UMaterial>(Object);
        break;
    case Asset::EAssetType::SkeletalMesh:
        mSkeletalMesh = CoreUObject::Cast<Asset::USkeletalMesh>(Object);
        mMesh = mSkeletalMesh != nullptr ? mSkeletalMesh->GetGeometry() : nullptr;
        break;
    case Asset::EAssetType::Mesh:
        mMesh = CoreUObject::Cast<Asset::UStaticMesh>(Object);
        break;
    default:
        break;
    }

    if (mTexture == nullptr && mSkeleton == nullptr && mMaterial == nullptr && mMesh == nullptr) {
        Error = "The asset does not match the specified type";

        return false;
    }

    auto* Device{static_cast<ID3D11Device*>(mRenderer->GetNativeDevice())};

    if (mTexture != nullptr) {
        if (!mTexture->CreateResources(Device)) {
            Error = "Cannot create texture preview resources";

            return false;
        }

        return true;
    }

    if (!mScene.Initialize(Error)) {
        return false;
    }

    if (mSkeleton != nullptr) {
        if (mSkeleton->GetGlobalBindPoses().empty()) {
            Error = "The skeleton has no preview bones";

            return false;
        }

        Synchronize();

        return true;
    }

    if (mMaterial != nullptr) {
        mPrimitiveMesh = CoreUObject::NewObject<Asset::UStaticMesh>();
        mPrimitiveMesh->SetMeshData(Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Sphere));
        mMesh = mPrimitiveMesh.get();
    }

    if (mMesh == nullptr || !mMesh->CreateResources(Device)) {
        Error = "Cannot create mesh preview resources";

        return false;
    }

    for (const Asset::FMeshMaterialSlot& Slot : mMesh->GetMeshData().GetMaterialSlots()) {
        Asset::UMaterial* Material{Slot.GetMaterialPath().empty() || mAssets == nullptr ? nullptr : mAssets->LoadMaterial(Core::FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};

        if (!Slot.GetMaterialPath().empty() && Material == nullptr) {
            return false;
        }

        mMaterials.push_back(Material);
    }

    Synchronize();

    return true;
}

bool FMeshViewerContent::IsReady() const {
    return mTexture != nullptr || (mScene.GetScene() != nullptr && (mMesh != nullptr || mSkeleton != nullptr));
}

const Core::FBox& FMeshViewerContent::GetBounds() const {
    return mBounds;
}

Engine::FSceneInterface* FMeshViewerContent::GetScene() const {
    return mScene.GetScene();
}

Asset::UTexture2D* FMeshViewerContent::GetTexture() const {
    return mTexture;
}

void FMeshViewerContent::Synchronize() {
    if (!mDirty) {
        return;
    }

    mDirty = false;

    if (mMesh == nullptr) {
        if (mSkeleton != nullptr && !mSkeleton->GetGlobalBindPoses().empty()) {
            const auto& Transforms{mSkeleton->GetGlobalBindPoses()};
            const Core::FVector First{Transforms.front().TransformPosition({})};
            mBounds = {First, First};

            for (const Core::FMatrix& Transform : Transforms) {
                const Core::FVector Joint{Transform.TransformPosition({})};

                mBounds.mMinimum = mBounds.mMinimum.Min(Joint);
                mBounds.mMaximum = mBounds.mMaximum.Max(Joint);
            }

            const float Radius{std::max(mBounds.mMinimum.Distance(mBounds.mMaximum) * 0.5f, 0.01f)};
            mSkeletonShapes = BuildBoneShapes(mSkeleton->GetBones(), Transforms, Radius);

            for (const FBoneShape& Shape : mSkeletonShapes) {
                const Core::FBox Bounds{Engine::GetEditorMesh(Shape.GetMesh()).GetBounds().Transformed(Shape.GetWorld())};

                mBounds.mMinimum = mBounds.mMinimum.Min(Bounds.mMinimum);
                mBounds.mMaximum = mBounds.mMaximum.Max(Bounds.mMaximum);
            }
        }

        return;
    }

    Engine::FPrimitiveSceneProxy Proxy{};
    Proxy.mId = mPrimitiveId;
    Proxy.mMesh = mMesh;
    Proxy.mBounds = mPose.GetRevision().IsValid() ? mPose.GetMeshData().GetBounds() : mMesh->GetBounds();
    Proxy.mMaterial = mMaterial;
    Proxy.mMaterials = mMaterials;

    if (mPose.GetRevision().IsValid()) {
        Proxy.mSkeletalMesh = mSkeletalMesh;
        Proxy.mSkinMatrices = mPose.GetSkinMatrices();
        Proxy.mPoseRevision = mPose.GetRevision();
    }

    mBounds = Proxy.mBounds;
    mScene.GetScene()->AddPrimitive(Proxy);
    mScene.Synchronize();
}

void FMeshViewerContent::BuildOverlay(Engine::FPrimitiveDrawInterface& Overlay, float) const {
    if (mMesh == nullptr && mSkeleton != nullptr) {
        for (const FBoneShape& Shape : mSkeletonShapes) {
            const Core::FVector4 Color{Shape.GetMesh() == Engine::EEditorMesh::Sphere ? Core::FVector4{0.88f, 0.92f, 1.0f, 1.0f} : Core::FVector4{0.49f, 0.7f, 0.9f, 1.0f}};

            Overlay.DrawMesh({Shape.GetMesh(), Shape.GetWorld(), Color});
        }
    }
}

Asset::UStaticMesh* FMeshViewerContent::GetMesh() const {
    return mMesh;
}

Asset::USkeletalMesh* FMeshViewerContent::GetSkeletalMesh() const {
    return mSkeletalMesh;
}

Asset::USkeleton* FMeshViewerContent::GetSkeleton() const {
    return mSkeleton;
}

Engine::FSkeletalMeshPose& FMeshViewerContent::GetPose() {
    return mPose;
}

Core::TArray<CoreUObject::TObjectRef<Asset::UMaterial>>& FMeshViewerContent::GetMaterials() {
    return mMaterials;
}

bool FMeshViewerContent::SetSkeleton(Asset::USkeleton* Skeleton) {
    Engine::FSkeletalMeshPose Pose{};

    if (Skeleton != nullptr && (mSkeletalMesh == nullptr || !Pose.Reset(*mSkeletalMesh, *Skeleton) || mRenderer == nullptr || !mSkeletalMesh->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice())))) {
        return false;
    }

    mSkeleton = Skeleton;
    mPose = std::move(Pose);
    Refresh();

    return true;
}

void FMeshViewerContent::Refresh() {
    mDirty = true;
    Synchronize();
}
