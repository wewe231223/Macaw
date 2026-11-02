/**
 * @file USkeletalMeshComponent.cpp
 * @brief 스켈레탈 메시 참조와 객체별 포즈·재질·직렬화를 구현합니다.
 * @ingroup Engine
 */
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/FAssetManager.h"
#include "Asset/UMaterial.h"
#include "Core/Archive.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/UWorld.h"

#include <utility>

namespace Engine {

    USkeletalMeshComponent::USkeletalMeshComponent() {
        SetEditorTickable(true);
    }

    USkeletalMeshComponent::~USkeletalMeshComponent() = default;

    Engine::EPrimitiveType USkeletalMeshComponent::GetPrimitiveType() const {
        return Engine::EPrimitiveType::Cube;
    }

    Asset::UStaticMesh* USkeletalMeshComponent::GetStaticMesh() const {
        return mSkeletalMesh != nullptr ? mSkeletalMesh->GetGeometry() : nullptr;
    }

    Asset::USkeletalMesh* USkeletalMeshComponent::GetSkeletalMesh() const {
        return mSkeletalMesh;
    }

    const Asset::FMeshData& USkeletalMeshComponent::GetMeshData() const {
        return mPose.GetMeshData();
    }

    const Core::FString& USkeletalMeshComponent::GetMeshPath() const {
        return mMeshPath;
    }

    bool USkeletalMeshComponent::SetMeshPath(const Core::FString& Path) {
        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};

        if (Path.find('\0') != Core::FString::npos || (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".masset"))) {
            return false;
        }

        for (const auto& Part : AssetPath) {
            if (Part == L"..") {
                return false;
            }
        }

        if (mMeshPath == Path && (Path.empty() || mSkeletalMesh != nullptr)) {
            return true;
        }

        Asset::USkeletalMesh* Mesh{LoadMesh(Path)};

        if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && Mesh == nullptr) {
            return false;
        }

        // 다른 에셋으로 바꾸면 이전 에셋의 본 번호와 슬롯 오버라이드는 폐기합니다.
        if (mMeshPath != Path) {
            mMaterialOverrides.clear();
            mPendingPose.clear();
            mPose = Engine::FSkeletalMeshPose{};
        }

        mMeshPath = Path;
        mSkeletalMesh = nullptr;
        ResolveMesh();
        MarkRenderStateDirty();
        return true;
    }

    bool USkeletalMeshComponent::ResolveMesh() {
        Asset::USkeletalMesh* Mesh{LoadMesh(mMeshPath)};

        if (Mesh != mSkeletalMesh || (Mesh != nullptr && mPose.GetMeshRevision() != Mesh->GetRevision())) {
            mSkeletalMesh = Mesh;
            mPose = Engine::FSkeletalMeshPose{};

            if (Mesh != nullptr && !mPose.Reset(*Mesh)) {
                mSkeletalMesh = nullptr;
                return false;
            }

            if (Mesh != nullptr && !mPendingPose.empty()) {
                if (!mPose.SetLocalTransforms(*Mesh, mPendingPose)) {
                    Core::ErrorHandler::Report("SkeletalMesh", "Stored pose does not match the skeletal mesh; using bind pose", Core::ErrorHandler::EErrorLevel::Warning);
                }

                mPendingPose.clear();
            }

            MarkRenderStateDirty();
        }

        return mMeshPath.empty() || mSkeletalMesh != nullptr;
    }

    const Engine::FSkeletalMeshPose& USkeletalMeshComponent::GetPose() const {
        return mPose;
    }

    bool USkeletalMeshComponent::SetBoneLocalTransform(Core::Uint32 Bone, const Core::FMatrix& Transform) {
        if (mSkeletalMesh == nullptr || !mPose.SetLocalTransform(*mSkeletalMesh, Bone, Transform)) {
            return false;
        }

        // 포즈 변경은 월드 변환이 그대로여도 프록시와 그림자 캐시를 갱신해야 합니다.
        MarkRenderStateDirty();
        return true;
    }

    bool USkeletalMeshComponent::SetLocalPose(const Core::TArray<Core::FMatrix>& Transforms) {
        if (mSkeletalMesh == nullptr || !mPose.SetLocalTransforms(*mSkeletalMesh, Transforms)) {
            return false;
        }

        MarkRenderStateDirty();
        return true;
    }

    bool USkeletalMeshComponent::ResetPose() {
        if (mSkeletalMesh == nullptr || !mPose.Reset(*mSkeletalMesh)) {
            return false;
        }

        MarkRenderStateDirty();
        return true;
    }

    bool USkeletalMeshComponent::ShouldCreateRenderState() const {
        return mSkeletalMesh != nullptr && mPose.GetSkinMatrices().size() == mSkeletalMesh->GetBones().size();
    }

    void USkeletalMeshComponent::OnRegister() {
        ResolveMesh();
        ResolveMaterial();
    }

    void USkeletalMeshComponent::UnregisterComponent() {
        // 복제·레벨 재연결 때 현재 로컬 포즈를 새 월드의 에셋에 다시 적용합니다.
        if (mSkeletalMesh != nullptr) {
            mPendingPose = mPose.GetLocalTransforms();
        }

        Engine::UPrimitiveComponent::UnregisterComponent();
        mSkeletalMesh = nullptr;
        mMaterial = nullptr;
        mPose = Engine::FSkeletalMeshPose{};
    }

    void USkeletalMeshComponent::Tick(float DeltaTime) {
        Engine::UPrimitiveComponent::Tick(DeltaTime);

        if (mSkeletalMesh != nullptr && mPose.GetMeshRevision() != mSkeletalMesh->GetRevision()) {
            ResolveMesh();
        }
    }

    Asset::USkeletalMesh* USkeletalMeshComponent::LoadMesh(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Asset::USkeletalMesh* Mesh{Assets->LoadSkeletalMesh(Core::FilePath::FromUtf8(Path), Error)};

        if (Mesh == nullptr) {
            Core::ErrorHandler::Report("SkeletalMesh", "Cannot load mesh '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Mesh;
    }

    const Core::FString& Engine::USkeletalMeshComponent::GetMaterialPath() const {
        return mMaterialPath;
    }

    bool Engine::USkeletalMeshComponent::SetMaterialPath(const Core::FString& Path) {
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

    Asset::UMaterial* Engine::USkeletalMeshComponent::GetMaterial() const {
        return mMaterial;
    }

    bool Engine::USkeletalMeshComponent::ResolveMaterial() {
        Asset::UMaterial* Material{LoadMaterial(mMaterialPath)};

        if (mMaterial != Material) {
            mMaterial = Material;
            MarkRenderStateDirty();
        }

        return mMaterialPath.empty() || mMaterial != nullptr;
    }

    Engine::FPrimitiveSceneProxy Engine::USkeletalMeshComponent::CreateSceneProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{Engine::UPrimitiveComponent::CreateSceneProxy()};
        Proxy.mMaterial = mMaterial;
        Proxy.mSkeletalMesh = mSkeletalMesh;
        Proxy.mSkinMatrices = mPose.GetSkinMatrices();
        Proxy.mPoseRevision = mPose.GetRevision();

        if (mSkeletalMesh != nullptr) {
            for (Core::Uint32 Slot{}; Slot < mSkeletalMesh->GetMeshData().GetMaterialSlots().size(); ++Slot) {
                Proxy.mMaterials.push_back(GetSlotMaterial(Slot));
            }
        }

        return Proxy;
    }

    const Core::FString& Engine::USkeletalMeshComponent::GetSlotMaterialPath(Core::Uint32 Slot) const {
        static const Core::FString Empty{};
        return Slot < mMaterialOverrides.size() ? mMaterialOverrides[Slot] : Empty;
    }

    Asset::UMaterial* Engine::USkeletalMeshComponent::GetSlotMaterial(Core::Uint32 Slot) const {
        if (mMaterial != nullptr) {
            return mMaterial;
        }

        if (!GetSlotMaterialPath(Slot).empty()) {
            return LoadMaterial(GetSlotMaterialPath(Slot));
        }

        return mSkeletalMesh != nullptr && Slot < mSkeletalMesh->GetMeshData().GetMaterialSlots().size() ? LoadMaterial(mSkeletalMesh->GetMeshData().GetMaterialSlots()[Slot].GetMaterialPath()) : nullptr;
    }

    bool Engine::USkeletalMeshComponent::SetSlotMaterialPath(Core::Uint32 Slot, const Core::FString& Path) {
        if (mSkeletalMesh == nullptr || Slot >= mSkeletalMesh->GetMeshData().GetMaterialSlots().size()) {
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

    void Engine::USkeletalMeshComponent::Serialize(Core::FArchive& Archive) {
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

        // 전역 행렬과 피킹 형상은 복원 시 다시 계산하고 로컬 포즈만 저장합니다.
        if (!Archive.HasError() && (!Archive.IsLoading() || Archive.HasField("BoneLocalTransforms"))) {
            Core::TArray<Core::FMatrix> Transforms{Archive.IsLoading() ? Core::TArray<Core::FMatrix>{} : mSkeletalMesh != nullptr ? mPose.GetLocalTransforms() : mPendingPose};
            Core::Uint32 BoneCount{static_cast<Core::Uint32>(Transforms.size())};

            if (Archive.BeginArray("BoneLocalTransforms", BoneCount)) {
                if (BoneCount > 100000) {
                    Archive.SetError("Invalid skeletal pose bone count");
                } else {
                    Transforms.resize(BoneCount);

                    for (Core::Uint32 Bone{}; Bone < BoneCount && !Archive.HasError(); ++Bone) {
                        Archive.Serialize(std::to_string(Bone), Transforms[Bone]);
                        Core::FMatrix Inverse{};

                        if (!Transforms[Bone].IsFinite() || !Transforms[Bone].TryInverse(Inverse) || !Inverse.IsFinite()) {
                            Archive.SetError("Invalid skeletal pose transform");
                        }
                    }
                }

                Archive.EndArray();
            }

            if (Archive.IsLoading() && !Archive.HasError()) {
                mPendingPose = std::move(Transforms);
            }
        }

        if (Archive.IsLoading()) {
            MarkRenderStateDirty();
        }
    }

    Asset::UMaterial* Engine::USkeletalMeshComponent::LoadMaterial(const Core::FString& Path) const {
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

}
