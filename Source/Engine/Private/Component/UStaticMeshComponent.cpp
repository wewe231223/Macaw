/**
 * @file UStaticMeshComponent.cpp
 * @brief Plane, Cube 또는 Sphere 프리미티브를 표시하는 메시 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UStaticMeshComponent.h"
#include "Core/FilePath.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FAssetManager.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UMaterial.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/UWorld.h"
#include "Core/Archive.h"

namespace Engine {

    /**
     * @brief UStaticMeshComponent의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Engine::UStaticMeshComponent::UStaticMeshComponent() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::UStaticMeshComponent::~UStaticMeshComponent() = default;

    /**
     * @brief 표시할 프리미티브 메시 종류를 반환합니다.
     */
    Engine::EPrimitiveType Engine::UStaticMeshComponent::GetPrimitiveType() const {
        return mPrimitive;
    }

    /**
     * @brief 지원되는 프리미티브 종류를 검증하고 렌더 상태 갱신을 예약합니다.
     */
    bool Engine::UStaticMeshComponent::SetPrimitiveType(Engine::EPrimitiveType Type) {
        if (Type >= Engine::EPrimitiveType::Count) {
            return false;
        }

        if (mPrimitive != Type) {
            mPrimitive = Type;
            MarkRenderStateDirty();
        }

        return true;
    }

    const Core::FString& Engine::UStaticMeshComponent::GetMeshPath() const {
        return mMeshPath;
    }

    bool Engine::UStaticMeshComponent::SetMeshPath(const Core::FString& Path) {
        if (Path.find('\0') != Core::FString::npos) {
            return false;
        }

        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};

        if (!Path.empty() && (AssetPath.is_absolute() || AssetPath.has_root_name() || AssetPath.extension() != L".masset")) {
            return false;
        }

        for (const auto& Part : AssetPath) {
            if (Part == L"..") {
                return false;
            }
        }

        if (mMeshPath == Path && (Path.empty() || mStaticMesh != nullptr)) {
            return true;
        }

        Engine::UStaticMesh* Mesh{LoadMesh(Path)};

        if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Mesh == nullptr) {
            return false;
        }

        if (mMeshPath != Path || mStaticMesh != Mesh) {
            if (mMeshPath != Path) {
                mMaterialOverrides.clear();
            }

            mMeshPath = Path;
            mStaticMesh = Mesh;
            MarkRenderStateDirty();
        }

        return true;
    }

    Engine::UStaticMesh* Engine::UStaticMeshComponent::GetStaticMesh() const {
        return mStaticMesh;
    }

    const Engine::FMeshData& Engine::UStaticMeshComponent::GetMeshData() const {
        if (mStaticMesh != nullptr) {
            return mStaticMesh->GetMeshData();
        }

        if (mMeshPath.empty()) {
            return Engine::UPrimitiveComponent::GetMeshData();
        }

        static const Engine::FMeshData EmptyMesh{};

        return EmptyMesh;
    }

    void Engine::UStaticMeshComponent::UnregisterComponent() {
        Engine::UWorld::FDispatchScope Scope{GetWorld()};

        Engine::UPrimitiveComponent::UnregisterComponent();
        mStaticMesh = nullptr;
        mMaterial = nullptr;
    }

    bool Engine::UStaticMeshComponent::ResolveMesh() {
        Engine::UStaticMesh* Mesh{LoadMesh(mMeshPath)};

        if (mStaticMesh != Mesh) {
            mStaticMesh = Mesh;
            MarkRenderStateDirty();
        }

        return mMeshPath.empty() || mStaticMesh != nullptr;
    }

    bool Engine::UStaticMeshComponent::ShouldCreateRenderState() const {
        return mMeshPath.empty() || mStaticMesh != nullptr;
    }

    void Engine::UStaticMeshComponent::OnRegister() {
        ResolveMesh();
        ResolveMaterial();
    }

    Engine::UStaticMesh* Engine::UStaticMeshComponent::LoadMesh(const Core::FString& Path) const {
        Engine::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Engine::UStaticMesh* Mesh{Assets->LoadMesh(Core::FilePath::FromUtf8(Path), Error)};

        if (Mesh == nullptr) {
            Core::ErrorHandler::Report("StaticMesh", "Cannot load mesh '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Mesh;
    }

    const Core::FString& Engine::UStaticMeshComponent::GetMaterialPath() const {
        return mMaterialPath;
    }

    bool Engine::UStaticMeshComponent::SetMaterialPath(const Core::FString& Path) {
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

        Engine::UMaterial* Material{LoadMaterial(Path)};

        if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Material == nullptr) {
            return false;
        }

        mMaterialPath = Path;
        mMaterial = Material;
        MarkRenderStateDirty();

        return true;
    }

    Engine::UMaterial* Engine::UStaticMeshComponent::GetMaterial() const {
        return mMaterial;
    }

    bool Engine::UStaticMeshComponent::ResolveMaterial() {
        Engine::UMaterial* Material{LoadMaterial(mMaterialPath)};

        if (mMaterial != Material) {
            mMaterial = Material;
            MarkRenderStateDirty();
        }

        return mMaterialPath.empty() || mMaterial != nullptr;
    }

    Engine::FPrimitiveSceneProxy Engine::UStaticMeshComponent::CreateSceneProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{Engine::UPrimitiveComponent::CreateSceneProxy()};
        Proxy.mMaterial = mMaterial;

        if (mStaticMesh != nullptr) {
            for (Core::Uint32 Slot{}; Slot < mStaticMesh->GetMeshData().GetMaterialSlots().size(); ++Slot) {
                Proxy.mMaterials.push_back(GetSlotMaterial(Slot));
            }
        }

        return Proxy;
    }

    const Core::FString& Engine::UStaticMeshComponent::GetSlotMaterialPath(Core::Uint32 Slot) const {
        static const Core::FString Empty{};
        return Slot < mMaterialOverrides.size() ? mMaterialOverrides[Slot] : Empty;
    }

    Engine::UMaterial* Engine::UStaticMeshComponent::GetSlotMaterial(Core::Uint32 Slot) const {
        if (mMaterial != nullptr) {
            return mMaterial;
        }

        if (!GetSlotMaterialPath(Slot).empty()) {
            return LoadMaterial(GetSlotMaterialPath(Slot));
        }

        return mStaticMesh != nullptr && Slot < mStaticMesh->GetMeshData().GetMaterialSlots().size() ? LoadMaterial(mStaticMesh->GetMeshData().GetMaterialSlots()[Slot].GetMaterialPath()) : nullptr;
    }

    bool Engine::UStaticMeshComponent::SetSlotMaterialPath(Core::Uint32 Slot, const Core::FString& Path) {
        if (mStaticMesh == nullptr || Slot >= mStaticMesh->GetMeshData().GetMaterialSlots().size()) {
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

    void Engine::UStaticMeshComponent::Serialize(Core::FArchive& Archive) {
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

    Engine::UMaterial* Engine::UStaticMeshComponent::LoadMaterial(const Core::FString& Path) const {
        Engine::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Engine::UMaterial* Material{Assets->LoadMaterial(Core::FilePath::FromUtf8(Path), Error)};

        if (Material == nullptr) {
            Core::ErrorHandler::Report("Material", "Cannot load material '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Material;
    }

}
