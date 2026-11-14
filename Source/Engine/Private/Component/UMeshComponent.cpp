/**
 * @file UMeshComponent.cpp
 * @brief 메시의 공통 머티리얼 참조와 슬롯별 덮어쓰기를 관리합니다.
 * @ingroup Engine
 */

#include "Core/Types.h"
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

    /**
     * @brief 전체 덮어쓰기 머티리얼의 프로젝트 상대 경로를 반환합니다.
     */
    const Core::FString& Engine::UMeshComponent::GetMaterialPath() const {
        return mMaterialPath;
    }

    /**
     * @brief 전체 덮어쓰기 머티리얼 경로를 검증·해석하고 렌더 상태를 갱신합니다.
     */
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

    /**
     * @brief 메시 전체에 적용할 덮어쓰기 머티리얼의 비소유 주소를 반환합니다.
     */
    Asset::UMaterial* Engine::UMeshComponent::GetMaterial() const {
        return mMaterial;
    }

    /**
     * @brief 저장한 전체 덮어쓰기 머티리얼 경로를 에셋 참조로 해석합니다.
     */
    bool Engine::UMeshComponent::ResolveMaterial() {
        Asset::UMaterial* Material{LoadMaterial(mMaterialPath)};

        if (mMaterial != Material) {
            mMaterial = Material;

            MarkRenderStateDirty();
        }

        return mMaterialPath.empty() || mMaterial != nullptr;
    }

    /**
     * @brief 컴포넌트의 현재 상태를 복사한 프리미티브 프록시를 만듭니다.
     */
    Engine::FPrimitiveSceneProxy Engine::UMeshComponent::CreateSceneProxy() const {
        Asset::UStaticMesh* Mesh{GetStaticMesh()};

        Engine::FPrimitiveSceneProxy Proxy{Engine::UPrimitiveComponent::CreateSceneProxy()};
        Proxy.mMaterial = mMaterial;

        if (Mesh != nullptr) {
            for (Core::Index_t Slot{}; Slot < Mesh->GetMeshData().GetMaterialSlots().size(); ++Slot) {
                Proxy.mMaterials.push_back(GetSlotMaterial(Slot));
            }
        }

        return Proxy;
    }

    /**
     * @brief 지정 슬롯의 덮어쓰기 머티리얼 경로를 반환합니다.
     */
    const Core::FString& Engine::UMeshComponent::GetSlotMaterialPath(Core::Index_t Slot) const {
        static const Core::FString Empty{};

        return Slot < mMaterialOverrides.size() ? mMaterialOverrides[Slot] : Empty;
    }

    /**
     * @brief 전체 덮어쓰기·슬롯 덮어쓰기·메시 기본값 순으로 슬롯 머티리얼을 구합니다.
     */
    Asset::UMaterial* Engine::UMeshComponent::GetSlotMaterial(Core::Index_t Slot) const {
        Asset::UStaticMesh* Mesh{GetStaticMesh()};

        if (mMaterial != nullptr) {
            return mMaterial;
        }

        if (!GetSlotMaterialPath(Slot).empty()) {
            return LoadMaterial(GetSlotMaterialPath(Slot));
        }

        return Mesh != nullptr && Slot < Mesh->GetMeshData().GetMaterialSlots().size() ? LoadMaterial(Mesh->GetMeshData().GetMaterialSlots()[Slot].GetMaterialPath()) : nullptr;
    }

    /**
     * @brief 지정 슬롯의 머티리얼 경로를 검증해 덮어쓰기에 반영합니다.
     */
    bool Engine::UMeshComponent::SetSlotMaterialPath(Core::Index_t Slot, const Core::FString& Path) {
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

    /**
     * @brief 슬롯별 머티리얼 덮어쓰기 경로 배열을 반환합니다.
     */
    const Core::TArray<Core::FString>& Engine::UMeshComponent::GetSlotMaterialPaths() const {
        return mMaterialOverrides;
    }

    /**
     * @brief 슬롯별 머티리얼 경로 배열을 검증해 덮어쓰기를 교체합니다.
     */
    bool Engine::UMeshComponent::SetSlotMaterialPaths(const Core::TArray<Core::FString>& Paths) {
        for (const Core::FString& Override : Paths) {
            const auto Path{Core::FilePath::FromUtf8(Override)};
            bool Valid{Override.find('\0') == Core::FString::npos && (Path.empty() || (!Path.has_root_path() && Path.extension() == L".masset"))};

            for (const auto& Part : Path) {
                Valid = Valid && Part != L"..";
            }

            if (!Valid) {
                return false;
            }
        }

        mMaterialOverrides = Paths;

        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 월드의 에셋 관리자에서 지정한 머티리얼을 조회합니다.
     */
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

    /**
     * @brief 월드에 등록된 컴포넌트가 참조하는 에셋과 렌더 상태를 준비합니다.
     */
    void Engine::UMeshComponent::OnRegister() {
        ResolveMesh();
        ResolveMaterial();
    }

    /**
     * @brief 슬롯별 머티리얼 경로를 비우고 렌더 상태를 갱신 대상으로 표시합니다.
     */
    void Engine::UMeshComponent::ClearMaterialOverrides() {
        if (!mMaterialOverrides.empty()) {
            mMaterialOverrides.clear();
            MarkRenderStateDirty();
        }
    }

    /**
     * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
     */
    void Engine::UMeshComponent::UnregisterComponent() {
        Engine::UWorld::FDispatchScope Scope{GetWorld()};

        Engine::UPrimitiveComponent::UnregisterComponent();

        mMaterial = nullptr;
    }

}
