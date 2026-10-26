/**
 * @file UStaticMeshComponent.cpp
 * @brief Plane, Cube 또는 Sphere 프리미티브를 표시하는 메시 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UStaticMeshComponent.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FAssetManager.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UMaterial.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/UWorld.h"

/**
 * @brief UStaticMeshComponent의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UStaticMeshComponent::UStaticMeshComponent() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UStaticMeshComponent::~UStaticMeshComponent() = default;

/**
 * @brief 표시할 프리미티브 메시 종류를 반환합니다.
 */
EPrimitiveType UStaticMeshComponent::GetPrimitiveType() const {
    return mPrimitive;
}

/**
 * @brief 지원되는 프리미티브 종류를 검증하고 렌더 상태 갱신을 예약합니다.
 */
bool UStaticMeshComponent::SetPrimitiveType(EPrimitiveType Type) {
    if (Type >= EPrimitiveType::Count) {
        return false;
    }

    if (mPrimitive != Type) {
        mPrimitive = Type;
        MarkRenderStateDirty();
    }

    return true;
}

const FString& UStaticMeshComponent::GetMeshPath() const {
    return mMeshPath;
}

bool UStaticMeshComponent::SetMeshPath(const FString& Path) {
    if (Path.find('\0') != FString::npos) {
        return false;
    }
    const std::filesystem::path AssetPath{std::u8string_view{reinterpret_cast<const char8_t*>(Path.data()), Path.size()}};
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

    UStaticMesh* Mesh{LoadMesh(Path)};
    if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Mesh == nullptr) {
        return false;
    }

    if (mMeshPath != Path || mStaticMesh != Mesh) {
        mMeshPath = Path;
        mStaticMesh = Mesh;
        MarkRenderStateDirty();
    }
    return true;
}

UStaticMesh* UStaticMeshComponent::GetStaticMesh() const {
    return mStaticMesh;
}

const FMeshData& UStaticMeshComponent::GetMeshData() const {
    if (mStaticMesh != nullptr) {
        return mStaticMesh->GetMeshData();
    }

    if (mMeshPath.empty()) {
        return UPrimitiveComponent::GetMeshData();
    }

    static const FMeshData EmptyMesh{};
    return EmptyMesh;
}

void UStaticMeshComponent::UnregisterComponent() {
    UWorld::FDispatchScope Scope{GetWorld()};
    UPrimitiveComponent::UnregisterComponent();
    mStaticMesh = nullptr;
    mMaterial = nullptr;
}

bool UStaticMeshComponent::ResolveMesh() {
    UStaticMesh* Mesh{LoadMesh(mMeshPath)};
    if (mStaticMesh != Mesh) {
        mStaticMesh = Mesh;
        MarkRenderStateDirty();
    }

    return mMeshPath.empty() || mStaticMesh != nullptr;
}

bool UStaticMeshComponent::ShouldCreateRenderState() const {
    return mMeshPath.empty() || mStaticMesh != nullptr;
}

void UStaticMeshComponent::OnRegister() {
    ResolveMesh();
    ResolveMaterial();
}

UStaticMesh* UStaticMeshComponent::LoadMesh(const FString& Path) const {
    FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};
    if (Assets == nullptr || Path.empty()) {
        return nullptr;
    }

    FString Error{};
    UStaticMesh* Mesh{Assets->LoadMesh(std::filesystem::path{std::u8string_view{reinterpret_cast<const char8_t*>(Path.data()), Path.size()}}, Error)};
    if (Mesh == nullptr) {
        ErrorHandler::Report("StaticMesh", "Cannot load mesh '" + Path + "': " + Error, ErrorHandler::EErrorLevel::Warning);
    }

    return Mesh;
}

const FString& UStaticMeshComponent::GetMaterialPath() const {
    return mMaterialPath;
}

bool UStaticMeshComponent::SetMaterialPath(const FString& Path) {
    if (Path.find('\0') != FString::npos) {
        return false;
    }
    const std::filesystem::path AssetPath{std::u8string_view{reinterpret_cast<const char8_t*>(Path.data()), Path.size()}};
    if (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".mtl")) {
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
    UMaterial* Material{LoadMaterial(Path)};
    if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Material == nullptr) {
        return false;
    }
    mMaterialPath = Path;
    mMaterial = Material;
    MarkRenderStateDirty();
    return true;
}

UMaterial* UStaticMeshComponent::GetMaterial() const {
    return mMaterial;
}

bool UStaticMeshComponent::ResolveMaterial() {
    UMaterial* Material{LoadMaterial(mMaterialPath)};
    if (mMaterial != Material) {
        mMaterial = Material;
        MarkRenderStateDirty();
    }
    return mMaterialPath.empty() || mMaterial != nullptr;
}

FPrimitiveSceneProxy UStaticMeshComponent::CreateSceneProxy() const {
    FPrimitiveSceneProxy Proxy{UPrimitiveComponent::CreateSceneProxy()};
    Proxy.mMaterial = mMaterial;
    return Proxy;
}

UMaterial* UStaticMeshComponent::LoadMaterial(const FString& Path) const {
    FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};
    if (Assets == nullptr || Path.empty()) {
        return nullptr;
    }
    FString Error{};
    UMaterial* Material{Assets->LoadMaterial(std::filesystem::path{std::u8string_view{reinterpret_cast<const char8_t*>(Path.data()), Path.size()}}, Error)};
    if (Material == nullptr) {
        ErrorHandler::Report("Material", "Cannot load material '" + Path + "': " + Error, ErrorHandler::EErrorLevel::Warning);
    }
    return Material;
}
