/**
 * @file FAssetThumbnailCache.cpp
 * @brief 공통 에셋 뷰어에서 촬영한 이미지를 캐시하고 변경된 에셋의 생성을 예약합니다.
 * @ingroup Editor
 */

#include "Editor/FAssetThumbnailCache.h"
#include "Viewer/FMeshViewerContent.h"
#include "Viewer/FAnimationViewerContent.h"
#include "Asset/UStaticMesh.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"
#include "Asset/UMaterial.h"
#include "Asset/UTexture2D.h"
#include "Asset/Animation/UAnimSequence.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/PrimitiveMesh.h"

#include "Asset/FAssetManager.h"
#include "Core/FilePath.h"
#include "Serialization/JsonArchive.h"

#include <algorithm>
#include <bit>
#include <functional>

using namespace Editor;

FAssetThumbnailCache::FThumbnail::FThumbnail() = default;
FAssetThumbnailCache::FThumbnail::~FThumbnail() = default;

FAssetThumbnailCache::FPendingThumbnail::FPendingThumbnail(const Core::FString& Key, const std::filesystem::path& Path, CoreUObject::UObject* Object, Asset::EAssetType Type, std::size_t Signature)
	: mKey{Key},
	  mPath{Path},
	  mObject{Object},
	  mType{Type},
	  mSignature{Signature} {
}

FAssetThumbnailCache::FAssetThumbnailCache(Asset::FAssetManager* Assets, RenderCore::IRendererModule* Renderer)
	: mAssets{Assets},
	  mRenderer{Renderer},
	  mDefaultMesh{CoreUObject::NewObject<Asset::UStaticMesh>()},
	  mDefaultMaterial{CoreUObject::NewObject<Asset::UMaterial>()},
	  mCaptureViewer{Renderer, Assets},
	  mMaterialViewer{Renderer, Assets} {
    mDefaultMesh->SetMeshData(Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Cube));
    LoadPreviewSettings();
}

FAssetThumbnailCache::~FAssetThumbnailCache() = default;

void FAssetThumbnailCache::BeginFrame() {
    ++mFrame;

    mRetiredCaptures.clear();

    while (mThumbnails.size() >= 128) {
        const auto Oldest{std::min_element(mThumbnails.begin(), mThumbnails.end(), [](const auto& First, const auto& Second) {
            return First.second.mLastUsed < Second.second.mLastUsed;
        })};

        if (Oldest == mThumbnails.end() || Oldest->second.mLastUsed + 1 >= mFrame) {
            break;
        }

        mThumbnails.erase(Oldest);
    }

    ProcessRequests();
}

void FAssetThumbnailCache::Invalidate() {
    ++mGeneration;
    mRequests.clear();
    mWorkerActive = false;
    mCaptureViewer.Close();

    for (auto& [Key, Thumbnail] : mThumbnails) {
        Thumbnail.mQueued = false;
    }
}


Core::FString FAssetThumbnailCache::MakeKey(const std::filesystem::path& Path, Asset::EAssetType Type) const {
    if (Path.empty()) {
        return "@Default" + std::to_string(static_cast<Core::Uint32>(Type));
    }

    Core::FString Error{};

    return Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(mAssets->GetRelativePath(Path, Error))));
}

void* FAssetThumbnailCache::GetThumbnail(const std::filesystem::path& Path, Asset::EAssetType Type) {
    if (mAssets == nullptr || mRenderer == nullptr || !Viewer::FAssetViewer::Supports(Type)) {
        return nullptr;
    }

    const Core::FString Key{MakeKey(Path, Type)};
    FThumbnail& Thumbnail{mThumbnails[Key]};
    Thumbnail.mLastUsed = mFrame;
    Core::FString Error{};
    CoreUObject::UObject* Object{LoadAsset(Path, Type, Error)};

    if (Object == nullptr) {
        return Thumbnail.mCapture != nullptr ? Thumbnail.mCapture->GetNativeTexture() : nullptr;
    }
    Core::FString MeshPath{};
    Core::FString SkeletonPath{};

    GetAnimationPreview(Path, MeshPath, SkeletonPath);
    const std::size_t Signature{mGeneration ^ AssetSignature(Object, Type) ^ std::hash<Core::FString>{}(MeshPath) ^ std::hash<Core::FString>{}(SkeletonPath)};

    if ((!Thumbnail.mHasSignature || Thumbnail.mSignature != Signature) && (!Thumbnail.mFailed || Thumbnail.mFailedSignature != Signature) && (!Thumbnail.mQueued || Thumbnail.mPendingSignature != Signature)) {
        Thumbnail.mPendingSignature = Signature;
        Thumbnail.mQueued = true;
        mRequests.emplace_back(Key, Path, Object, Type, Signature);
    }

    return Thumbnail.mCapture != nullptr ? Thumbnail.mCapture->GetNativeTexture() : nullptr;
}

void FAssetThumbnailCache::ProcessRequests() {
    while (!mRequests.empty()) {
        const FPendingThumbnail& Pending{mRequests.front()};
        const auto Found{mThumbnails.find(Pending.mKey)};

        if (Found == mThumbnails.end() || !Found->second.mQueued || Found->second.mPendingSignature != Pending.mSignature) {
            mWorkerActive = false;
            mRequests.pop_front();

            continue;
        }

        FThumbnail& Thumbnail{Found->second};
        Core::FString Error{};
        const bool Opened{mWorkerActive || BindCaptureAsset(Pending, Error)};
        Viewer::EViewerPrepareResult Prepared{Viewer::EViewerPrepareResult::Failed};

        if (Opened) {
            mWorkerActive = true;
            Prepared = mCaptureViewer.PrepareCapture(Error);
        }

        if (Prepared == Viewer::EViewerPrepareResult::Pending) {
            break;
        }

        std::unique_ptr<Viewer::FViewerCapture> Capture{};

        if (Prepared == Viewer::EViewerPrepareResult::Ready && mCaptureViewer.Resize(256, 256) && mCaptureViewer.Fit()) {
            Capture = mCaptureViewer.Capture(Error);
        }

        if (Capture != nullptr) {
            if (Thumbnail.mCapture != nullptr) {
                mRetiredCaptures.push_back(std::move(Thumbnail.mCapture));
            }

            Thumbnail.mCapture = std::move(Capture);
            Thumbnail.mSignature = Pending.mSignature;
            Thumbnail.mHasSignature = true;
            Thumbnail.mFailed = false;
        } else {
            Thumbnail.mFailedSignature = Pending.mSignature;
            Thumbnail.mFailed = true;
        }

        Thumbnail.mQueued = false;
        mWorkerActive = false;
        mRequests.pop_front();
    }
}

void* FAssetThumbnailCache::GetMaterialPreview(Asset::UMaterial* Material) {
    FThumbnail& Thumbnail{mThumbnails["@MaterialPreview"]};
    Thumbnail.mLastUsed = mFrame;
    const std::size_t Signature{MaterialSignature(Material) ^ mGeneration};

    if (mRenderer != nullptr && (!Thumbnail.mHasSignature || Thumbnail.mSignature != Signature) && (!Thumbnail.mFailed || Thumbnail.mFailedSignature != Signature)) {
        Core::FString Error{};
        std::unique_ptr<Viewer::FViewerCapture> Capture{};

        if (mMaterialViewer.SetAsset(Material != nullptr ? Material : mDefaultMaterial.get(), Asset::EAssetType::Material) && mMaterialViewer.Resize(256, 256) && mMaterialViewer.Fit()) {
            Capture = mMaterialViewer.Capture(Error);
        }

        if (Capture != nullptr) {
            if (Thumbnail.mCapture != nullptr) {
                mRetiredCaptures.push_back(std::move(Thumbnail.mCapture));
            }

            Thumbnail.mCapture = std::move(Capture);
            Thumbnail.mSignature = Signature;
            Thumbnail.mHasSignature = true;
            Thumbnail.mFailed = false;
        } else {
            Thumbnail.mFailedSignature = Signature;
            Thumbnail.mFailed = true;
        }
    }

    return Thumbnail.mCapture != nullptr ? Thumbnail.mCapture->GetNativeTexture() : nullptr;
}

bool FAssetThumbnailCache::SetAnimationPreview(const std::filesystem::path& Path, const Core::FString& MeshPath, const Core::FString& SkeletonPath, Core::FString& Error) {
    Error.clear();
    const Core::FString Key{MakeKey(Path, Asset::EAssetType::AnimSequence)};
    const auto Found{mAnimationPreviews.find(Key)};

    if (Found != mAnimationPreviews.end() && Found->second.first == MeshPath && Found->second.second == SkeletonPath) {
        return true;
    }

    if (MeshPath.empty()) {
        mAnimationPreviews.erase(Key);
    } else {
        mAnimationPreviews[Key] = {MeshPath, SkeletonPath};
    }

    Invalidate();

    if (!SavePreviewSettings()) {
        Error = "Cannot save animation preview settings";

        return false;
    }

    return true;
}

bool FAssetThumbnailCache::GetAnimationPreview(const std::filesystem::path& Path, Core::FString& MeshPath, Core::FString& SkeletonPath) const {
    const auto Found{mAnimationPreviews.find(MakeKey(Path, Asset::EAssetType::AnimSequence))};

    if (Found == mAnimationPreviews.end()) {
        return false;
    }

    MeshPath = Found->second.first;
    SkeletonPath = Found->second.second;

    return true;
}

void FAssetThumbnailCache::LoadPreviewSettings() {
    if (mAssets == nullptr) {
        return;
    }

    Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};
    const std::filesystem::path Path{mAssets->GetContentRoot().parent_path() / "Saved/ViewerPreviews.json"};
    Core::Uint32 Count{};

    if (!Archive.Load(Path) || !Archive.BeginArray("Animations", Count) || Count > 10000) {
        return;
    }

    for (Core::Uint32 Index{}; Index < Count; ++Index) {
        if (Archive.BeginObject(std::to_string(Index))) {
            Core::FString AssetPath{};
            Core::FString MeshPath{};
            Core::FString SkeletonPath{};
            Archive.Serialize("Asset", AssetPath);
            Archive.Serialize("Mesh", MeshPath);
            Archive.Serialize("Skeleton", SkeletonPath);
            Archive.EndObject();

            if (!AssetPath.empty() && !MeshPath.empty()) {
                mAnimationPreviews[AssetPath] = {MeshPath, SkeletonPath};
            }
        }
    }

    Archive.EndArray();
}

bool FAssetThumbnailCache::SavePreviewSettings() {
    if (mAssets == nullptr) {
        return false;
    }

    Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};
    Core::Uint32 Count{static_cast<Core::Uint32>(mAnimationPreviews.size())};
    Archive.BeginArray("Animations", Count);
    Core::Uint32 Index{};

    for (const auto& [Key, Preview] : mAnimationPreviews) {
        Archive.BeginObject(std::to_string(Index++));
        Core::FString AssetPath{Key};
        Core::FString MeshPath{Preview.first};
        Core::FString SkeletonPath{Preview.second};
        Archive.Serialize("Asset", AssetPath);
        Archive.Serialize("Mesh", MeshPath);
        Archive.Serialize("Skeleton", SkeletonPath);
        Archive.EndObject();
    }

    Archive.EndArray();

    return Archive.Save(mAssets->GetContentRoot().parent_path() / "Saved/ViewerPreviews.json");
}

std::size_t FAssetThumbnailCache::MaterialSignature(const Asset::UMaterial* Material) {
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

CoreUObject::UObject* FAssetThumbnailCache::LoadAsset(const std::filesystem::path& Path, Asset::EAssetType Type, Core::FString& Error) {
    switch (Type) {
    case Asset::EAssetType::Mesh:
        return Path.empty() ? mDefaultMesh.get() : mAssets->LoadMesh(Path, Error);
    case Asset::EAssetType::SkeletalMesh:
        return mAssets->LoadSkeletalMesh(Path, Error);
    case Asset::EAssetType::Material:
        return Path.empty() ? mDefaultMaterial.get() : mAssets->LoadMaterial(Path, Error);
    case Asset::EAssetType::Skeleton:
        return mAssets->LoadSkeleton(Path, Error);
    case Asset::EAssetType::Texture:
        return mAssets->LoadTexture(Path, Error);
    case Asset::EAssetType::AnimSequence:
        return mAssets->LoadAnimSequence(Path, Error);
    default:
        return nullptr;
    }
}

std::size_t FAssetThumbnailCache::AssetSignature(CoreUObject::UObject* Object, Asset::EAssetType Type) {
    std::size_t Signature{std::hash<const void*>{}(Object) ^ static_cast<std::size_t>(Type)};

    if (auto* Material{CoreUObject::Cast<Asset::UMaterial>(Object)}) {
        Signature ^= MaterialSignature(Material);
    }

    auto* Skeletal{CoreUObject::Cast<Asset::USkeletalMesh>(Object)};
    Asset::UStaticMesh* Mesh{Skeletal != nullptr ? Skeletal->GetGeometry() : CoreUObject::Cast<Asset::UStaticMesh>(Object)};
    Core::FString Error{};

    if (Mesh != nullptr) {
        for (const Asset::FMeshMaterialSlot& Slot : Mesh->GetMeshData().GetMaterialSlots()) {
            Asset::UMaterial* Material{Slot.GetMaterialPath().empty() ? nullptr : mAssets->LoadMaterial(Core::FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};
            const std::size_t Value{MaterialSignature(Material)};
            Signature ^= Value + 0x9E3779B9 + (Signature << 6) + (Signature >> 2);
        }
    }

    return Signature;
}

bool FAssetThumbnailCache::BindCaptureAsset(const FPendingThumbnail& Pending, Core::FString& Error) {
    if (!mCaptureViewer.SetAsset(Pending.mObject, Pending.mType)) {
        Error = mCaptureViewer.GetError();

        return false;
    }

    if (Pending.mType == Asset::EAssetType::SkeletalMesh) {
        auto* Content{static_cast<Viewer::FMeshViewerContent*>(mCaptureViewer.GetContent())};

        return Content->SetSkeleton(mAssets->FindLegacySkeleton(Pending.mPath));
    }

    if (Pending.mType == Asset::EAssetType::AnimSequence) {
        Core::FString MeshPath{};
        Core::FString SkeletonPath{};

        if (!GetAnimationPreview(Pending.mPath, MeshPath, SkeletonPath)) {
            return false;
        }

        Asset::USkeletalMesh* Mesh{mAssets->LoadSkeletalMesh(Core::FilePath::FromUtf8(MeshPath), Error)};
        Asset::USkeleton* Skeleton{SkeletonPath.empty() ? mAssets->FindLegacySkeleton(Core::FilePath::FromUtf8(MeshPath)) : mAssets->LoadSkeleton(Core::FilePath::FromUtf8(SkeletonPath), Error)};
        auto* Content{static_cast<Viewer::FAnimationViewerContent*>(mCaptureViewer.GetContent())};

        return Mesh != nullptr && Content->SetMesh(Mesh, Skeleton, Error);
    }

    return true;
}
