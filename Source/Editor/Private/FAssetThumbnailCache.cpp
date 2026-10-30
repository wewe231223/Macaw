/**
 * @file FAssetThumbnailCache.cpp
 * @brief 구형 재질과 정적 메시 썸네일의 GPU 캐시 구현입니다.
 * @ingroup Editor
 */
#include "Editor/FAssetThumbnailCache.h"
#include "Core/FilePath.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/FAssetManager.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/PrimitiveMesh.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UMaterial.h"
#include "Engine/UTexture2D.h"
#include "RenderCore/IRendererModule.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cwctype>

namespace Editor {

    Editor::FAssetThumbnailCache::FAssetThumbnailCache(Engine::FAssetManager* Assets, RenderCore::IRendererModule* Renderer)
		: mAssets{Assets},
		  mRenderer{Renderer},
		  mSphere{CoreUObject::NewObject<Engine::UStaticMesh>()},
		  mCube{CoreUObject::NewObject<Engine::UStaticMesh>()} {
        mSphere->SetMeshData(Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Sphere));
        mCube->SetMeshData(Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Cube));
    }

    Editor::FAssetThumbnailCache::~FAssetThumbnailCache() = default;

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

    void Editor::FAssetThumbnailCache::Invalidate() {
        ++mGeneration;
    }

    std::size_t Editor::FAssetThumbnailCache::MaterialSignature(const Engine::UMaterial* Material) {
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

    void* Editor::FAssetThumbnailCache::GetThumbnail(const std::filesystem::path& Path, Engine::EAssetType Type) {
        if (mAssets == nullptr || mRenderer == nullptr) {
            return nullptr;
        }

        Core::FString Error{};

        if (Type == Engine::EAssetType::Texture) {
            Engine::UTexture2D* Texture{mAssets->LoadTexture(Path, Error)};
            return Texture != nullptr && Texture->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice())) ? Texture->GetShaderResourceView() : nullptr;
        }

        std::wstring KeyPath{Path.empty() ? std::wstring{} : Core::FilePath::FromUtf8(mAssets->GetRelativePath(Path, Error)).generic_wstring()};
        std::transform(KeyPath.begin(), KeyPath.end(), KeyPath.begin(), [](wchar_t Value) {
            return static_cast<wchar_t>(std::towlower(Value));
        });

        const Core::FString Key{Path.empty() ? Type == Engine::EAssetType::Mesh ? "@DefaultMesh" : "@DefaultMaterial" : Core::FilePath::ToUtf8(std::filesystem::path{KeyPath})};

        if (Type == Engine::EAssetType::Material) {
            Engine::UMaterial* Material{Path.empty() ? nullptr : mAssets->LoadMaterial(Path, Error)};
            return !Path.empty() && Material == nullptr ? nullptr : Render(Key, mSphere.get(), Material, MaterialSignature(Material) ^ mGeneration);
        }

        if (Type != Engine::EAssetType::Mesh) {
            return nullptr;
        }

        Engine::UStaticMesh* Mesh{Path.empty() ? mCube.get() : mAssets->LoadMesh(Path, Error)};

        if (Mesh == nullptr) {
            return nullptr;
        }

        std::size_t Signature{mGeneration};

        for (const Engine::FMeshMaterialSlot& Slot : Mesh->GetMeshData().GetMaterialSlots()) {
            Engine::UMaterial* Material{Slot.GetMaterialPath().empty() ? nullptr : mAssets->LoadMaterial(Core::FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};
            const std::size_t Value{MaterialSignature(Material)};
            Signature ^= Value + 0x9E3779B9 + (Signature << 6) + (Signature >> 2);
        }

        return Render(Key, Mesh, nullptr, Signature);
    }

    void* Editor::FAssetThumbnailCache::GetMaterialPreview(Engine::UMaterial* Material) {
        return mRenderer != nullptr ? Render("@MaterialPreview", mSphere.get(), Material, MaterialSignature(Material) ^ mGeneration) : nullptr;
    }

    void* Editor::FAssetThumbnailCache::Render(const Core::FString& Key, Engine::UStaticMesh* Mesh, Engine::UMaterial* Material, std::size_t Signature) {
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

        if (mScene == nullptr || Mesh == nullptr || !Mesh->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice()))) {
            return nullptr;
        }

        if (Thumbnail.mTarget == nullptr) {
            Thumbnail.mTarget = mRenderer->CreateRenderTarget(256, 256);
        }

        if (Thumbnail.mTarget == nullptr) {
            return nullptr;
        }

        Engine::FPrimitiveSceneProxy Proxy{};
        Proxy.mId = mPrimitiveId;
        Proxy.mMesh = Mesh;
        Proxy.mBounds = Mesh->GetBounds();
        Proxy.mMaterial = Material;

        Core::FString Error{};

        for (const Engine::FMeshMaterialSlot& Slot : Mesh->GetMeshData().GetMaterialSlots()) {
            Proxy.mMaterials.push_back(mAssets != nullptr && !Slot.GetMaterialPath().empty() ? mAssets->LoadMaterial(Core::FilePath::FromUtf8(Slot.GetMaterialPath()), Error) : nullptr);
        }

        mScene->AddPrimitive(Proxy);

        const Core::FVector Center{(Mesh->GetBounds().mMinimum + Mesh->GetBounds().mMaximum) * 0.5f};
        const float Radius{std::max(Mesh->GetBounds().mMinimum.Distance(Mesh->GetBounds().mMaximum) * 0.5f, 0.01f)};

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
        mRenderer->BeginRenderingViewFamily({mScene.get(), Thumbnail.mTarget.get(), View});
        Thumbnail.mSignature = Signature;

        return Thumbnail.mTarget->GetNativeTexture();
    }

}
