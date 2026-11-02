#include "Engine/Component/UMeshComponent.h"
#include "Asset/FAssetManager.h"
#include "Asset/UMaterial.h"
#include "Asset/UStaticMesh.h"
#include "Core/Archive.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/FilePath.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/UWorld.h"

namespace Engine {

    const Core::FString& Engine::UMeshComponent::GetMaterialPath() const {
        return mMaterialPath;
    }

    bool Engine::UMeshComponent::SetMaterialPath(const Core::FString& Path) {
        if (Path.find('\0') != Core::FString::npos) {
            return false;
        }

        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};

        if (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".masset")) {
            return false;
        }

        for (const auto& Part : AssetPath) {
            if (Part == L"..") {
                return false;
            }
        }

        if (mMaterialPath == Path && (Path.empty() || mMaterial != nullptr)) {
            return true;
        }

        Asset::UMaterial* Material{LoadMaterial(Path)};

        if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Material == nullptr) {
            return false;
        }

        mMaterialPath = Path;
        mMaterial = Material;
        MarkRenderStateDirty();

        return true;
    }

    Asset::UMaterial* Engine::UMeshComponent::GetMaterial() const {
        return mMaterial;
    }

    bool Engine::UMeshComponent::ResolveMaterial() {
        Asset::UMaterial* Material{LoadMaterial(mMaterialPath)};

        if (mMaterial != Material) {
            mMaterial = Material;
            MarkRenderStateDirty();
        }

        return mMaterialPath.empty() || mMaterial != nullptr;
    }

    Engine::FPrimitiveSceneProxy Engine::UMeshComponent::CreateSceneProxy() const {
        Asset::UStaticMesh* Mesh{GetStaticMesh()};

        Engine::FPrimitiveSceneProxy Proxy{Engine::UPrimitiveComponent::CreateSceneProxy()};
        Proxy.mMaterial = mMaterial;

        if (Mesh != nullptr) {
            for (Core::Uint32 Slot{}; Slot < Mesh->GetMeshData().GetMaterialSlots().size(); ++Slot) {
                Proxy.mMaterials.push_back(GetSlotMaterial(Slot));
            }
        }

        return Proxy;
    }

    const Core::FString& Engine::UMeshComponent::GetSlotMaterialPath(Core::Uint32 Slot) const {
        static const Core::FString Empty{};
        return Slot < mMaterialOverrides.size() ? mMaterialOverrides[Slot] : Empty;
    }

    Asset::UMaterial* Engine::UMeshComponent::GetSlotMaterial(Core::Uint32 Slot) const {
        Asset::UStaticMesh* Mesh{GetStaticMesh()};

        if (mMaterial != nullptr) {
            return mMaterial;
        }

        if (!GetSlotMaterialPath(Slot).empty()) {
            return LoadMaterial(GetSlotMaterialPath(Slot));
        }

        return Mesh != nullptr && Slot < Mesh->GetMeshData().GetMaterialSlots().size() ? LoadMaterial(Mesh->GetMeshData().GetMaterialSlots()[Slot].GetMaterialPath()) : nullptr;
    }

    bool Engine::UMeshComponent::SetSlotMaterialPath(Core::Uint32 Slot, const Core::FString& Path) {
        Asset::UStaticMesh* Mesh{GetStaticMesh()};

        if (Mesh == nullptr || Slot >= Mesh->GetMeshData().GetMaterialSlots().size()) {
            return false;
        }

        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};

        if (Path.find('\0') != Core::FString::npos || (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".masset" || LoadMaterial(Path) == nullptr))) {
            return false;
        }

        for (const auto& Part : AssetPath) {
            if (Part == L"..") {
                return false;
            }
        }

        if (mMaterialOverrides.size() <= Slot) {
            mMaterialOverrides.resize(Slot + 1);
        }

        mMaterialOverrides[Slot] = Path;
        MarkRenderStateDirty();

        return true;
    }

    void Engine::UMeshComponent::Serialize(Core::FArchive& Archive) {
        Engine::UPrimitiveComponent::Serialize(Archive);

        if (Archive.HasError() || (Archive.IsLoading() && !Archive.HasField("SlotMaterials"))) {
            return;
        }

        Core::Uint32 Count{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(mMaterialOverrides.size())};

        if (!Archive.BeginArray("SlotMaterials", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            mMaterialOverrides.resize(Count);
        }

        for (Core::Uint32 Slot{}; Slot < Count && !Archive.HasError(); ++Slot) {
            Archive.Serialize(std::to_string(Slot), mMaterialOverrides[Slot]);

            const auto Path{Core::FilePath::FromUtf8(mMaterialOverrides[Slot])};
            bool Valid{mMaterialOverrides[Slot].find('\0') == Core::FString::npos && (Path.empty() || (!Path.has_root_path() && Path.extension() == L".masset"))};

            for (const auto& Part : Path) {
                Valid = Valid && Part != L"..";
            }

            if (!Valid) {
                Archive.SetError("Invalid slot material reference");
            }
        }

        Archive.EndArray();

        if (Archive.IsLoading()) {
            MarkRenderStateDirty();
        }
    }

    Asset::UMaterial* Engine::UMeshComponent::LoadMaterial(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Asset::UMaterial* Material{Assets->LoadMaterial(Core::FilePath::FromUtf8(Path), Error)};

        if (Material == nullptr) {
            Core::ErrorHandler::Report("Material", "Cannot load material '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Material;
    }

    void Engine::UMeshComponent::OnRegister() {
        ResolveMesh();
        ResolveMaterial();
    }

    void Engine::UMeshComponent::ClearMaterialOverrides() {
        if (!mMaterialOverrides.empty()) {
            mMaterialOverrides.clear();
            MarkRenderStateDirty();
        }
    }

    void Engine::UMeshComponent::UnregisterComponent() {
        Engine::UWorld::FDispatchScope Scope{GetWorld()};

        Engine::UPrimitiveComponent::UnregisterComponent();
        mMaterial = nullptr;
    }

}
