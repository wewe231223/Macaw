/**
 * @file UStaticMeshComponent.cpp
 * @brief 기본 프리미티브 또는 정적 메시 에셋을 머티리얼과 함께 표시하는 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UStaticMeshComponent.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/EPrimitiveType.h"
#include "Asset/FAssetManager.h"
#include "Asset/UStaticMesh.h"
#include "Engine/UWorld.h"

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

        Asset::UStaticMesh* Mesh{LoadMesh(Path)};

        if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Mesh == nullptr) {
            return false;
        }

        if (mMeshPath != Path || mStaticMesh != Mesh) {
            if (mMeshPath != Path) {
                ClearMaterialOverrides();
            }

            mMeshPath = Path;
            mStaticMesh = Mesh;
            MarkRenderStateDirty();
        }

        return true;
    }

    Asset::UStaticMesh* Engine::UStaticMeshComponent::GetStaticMesh() const {
        return mStaticMesh;
    }

    const Asset::FMeshData& Engine::UStaticMeshComponent::GetMeshData() const {
        if (mStaticMesh != nullptr) {
            return mStaticMesh->GetMeshData();
        }

        if (mMeshPath.empty()) {
            return Engine::UPrimitiveComponent::GetMeshData();
        }

        static const Asset::FMeshData EmptyMesh{};

        return EmptyMesh;
    }

    void Engine::UStaticMeshComponent::UnregisterComponent() {
        Engine::UWorld::FDispatchScope Scope{GetWorld()};

        Engine::UMeshComponent::UnregisterComponent();
        mStaticMesh = nullptr;
    }

    bool Engine::UStaticMeshComponent::ResolveMesh() {
        Asset::UStaticMesh* Mesh{LoadMesh(mMeshPath)};

        if (mStaticMesh != Mesh) {
            mStaticMesh = Mesh;
            MarkRenderStateDirty();
        }

        return mMeshPath.empty() || mStaticMesh != nullptr;
    }

    bool Engine::UStaticMeshComponent::ShouldCreateRenderState() const {
        return mMeshPath.empty() || mStaticMesh != nullptr;
    }

    Asset::UStaticMesh* Engine::UStaticMeshComponent::LoadMesh(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Asset::UStaticMesh* Mesh{Assets->LoadMesh(Core::FilePath::FromUtf8(Path), Error)};

        if (Mesh == nullptr) {
            Core::ErrorHandler::Report("StaticMesh", "Cannot load mesh '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Mesh;
    }

}
