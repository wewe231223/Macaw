/**
 * @file USkeletalMeshComponent.cpp
 * @brief 스켈레탈 메시 참조와 객체별 포즈·재질·직렬화를 구현합니다.
 * @ingroup Engine
 */

#include "Engine/Component/USkeletalMeshComponent.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/FAssetManager.h"
#include "Core/Archive.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/UWorld.h"

#include <utility>

namespace Engine {

    /**
     * @brief 에디터에서도 에셋 변경을 감지하는 컴포넌트를 생성합니다.
     */
    USkeletalMeshComponent::USkeletalMeshComponent() {
        SetEditorTickable(true);
    }

    USkeletalMeshComponent::~USkeletalMeshComponent() = default;

    /**
     * @brief 기본 프리미티브 구분값을 반환합니다.
     */
    Engine::EPrimitiveType USkeletalMeshComponent::GetPrimitiveType() const {
        return Engine::EPrimitiveType::Cube;
    }

    /**
     * @brief 공유 바인드 형상을 반환합니다.
     */
    Asset::UStaticMesh* USkeletalMeshComponent::GetStaticMesh() const {
        // 공통 드로 경로에는 바인드 형상 버퍼를 넘기며, 변형은 정점 셰이더가 본 팔레트로 처리합니다.
        return mSkeletalMesh != nullptr ? mSkeletalMesh->GetGeometry() : nullptr;
    }

    /**
     * @brief 연결된 스켈레탈 메시를 반환합니다.
     */
    Asset::USkeletalMesh* USkeletalMeshComponent::GetSkeletalMesh() const {
        return mSkeletalMesh;
    }

    /**
     * @brief 현재 변형 위치와 경계를 가진 CPU 형상을 반환합니다.
     */
    const Asset::FMeshData& USkeletalMeshComponent::GetMeshData() const {
        // 피킹이 실제 보이는 표면과 일치하도록 에셋의 바인드 형상이 아니라 현재 포즈로 변형한 형상을 반환합니다.
        return mPose.GetMeshData();
    }

    /**
     * @brief 프로젝트 상대 에셋 경로를 반환합니다.
     */
    const Core::FString& USkeletalMeshComponent::GetMeshPath() const {
        return mMeshPath;
    }

    /**
     * @brief 에셋 경로를 설정하고 포즈를 초기화합니다.
     */
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
            ClearMaterialOverrides();

            mPose = Engine::FSkeletalMeshPose{};
        }

        mMeshPath = Path;
        mSkeletalMesh = nullptr;

        ResolveMesh();
        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 저장된 에셋 경로와 현재 에셋 개정을 해석합니다.
     */
    bool USkeletalMeshComponent::ResolveMesh() {
        Asset::USkeletalMesh* Mesh{LoadMesh(mMeshPath)};

        if (Mesh != mSkeletalMesh || (Mesh != nullptr && mPose.GetMeshRevision() != Mesh->GetRevision())) {
            const bool MeshReimported{Mesh != nullptr && mPose.GetMeshRevision().IsValid() && mPose.GetMeshRevision() != Mesh->GetRevision()};

            mSkeletalMesh = Mesh;
            mPose.Invalidate();

            if (Mesh != nullptr) {
                bool PoseReady{false};

                if (!MeshReimported && !mPose.GetLocalTransforms().empty()) {
                    PoseReady = mPose.Recalculate(*Mesh);

                    if (!PoseReady) {
                        Core::ErrorHandler::Report("SkeletalMesh", "Stored pose does not match the skeletal mesh; using bind pose", Core::ErrorHandler::EErrorLevel::Warning);
                    }
                }

                if (!PoseReady && !mPose.Reset(*Mesh)) {
                    mSkeletalMesh = nullptr;

                    return false;
                }
            }

            MarkRenderStateDirty();
        }

        return mMeshPath.empty() || mSkeletalMesh != nullptr;
    }

    /**
     * @brief 현재 본 포즈를 반환합니다.
     */
    const Engine::FSkeletalMeshPose& USkeletalMeshComponent::GetPose() const {
        return mPose;
    }

    /**
     * @brief 본 하나의 부모 기준 변환을 변경합니다.
     */
    bool USkeletalMeshComponent::SetBoneLocalTransform(Core::Uint32 Bone, const Core::FMatrix& Transform) {
        if (mSkeletalMesh == nullptr || !mPose.SetLocalTransform(*mSkeletalMesh, Bone, Transform)) {
            return false;
        }

        // 포즈 변경은 월드 변환이 그대로여도 프록시와 그림자 캐시를 갱신해야 합니다.
        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 전체 로컬 포즈를 변경합니다.
     */
    bool USkeletalMeshComponent::SetLocalPose(const Core::TArray<Core::FMatrix>& Transforms) {
        const bool Updated{mSkeletalMesh != nullptr ? mPose.SetLocalTransforms(*mSkeletalMesh, Transforms) : mPose.SetLocalTransforms(Transforms)};

        if (!Updated) {
            return false;
        }

        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 모든 본을 바인드 포즈로 복원합니다.
     */
    bool USkeletalMeshComponent::ResetPose() {
        if (mSkeletalMesh == nullptr || !mPose.Reset(*mSkeletalMesh)) {
            return false;
        }

        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 에셋과 포즈가 준비되었는지 검사합니다.
     */
    bool USkeletalMeshComponent::ShouldCreateRenderState() const {
        return mSkeletalMesh != nullptr && mPose.GetMeshRevision() == mSkeletalMesh->GetRevision() && mPose.GetSkinMatrices().size() == mSkeletalMesh->GetBones().size();
    }

    /**
     * @brief 재등록 때 재해석하도록 에셋 참조를 해제합니다.
     */
    void USkeletalMeshComponent::UnregisterComponent() {
        Engine::UWorld::FDispatchScope Scope{GetWorld()};

        Engine::UMeshComponent::UnregisterComponent();

        mSkeletalMesh = nullptr;
        mPose.Invalidate();
    }

    /**
     * @brief 재임포트에 따른 포즈·버퍼 갱신을 예약합니다.
     */
    void USkeletalMeshComponent::Tick(float DeltaTime) {
        Engine::UMeshComponent::Tick(DeltaTime);

        if (mSkeletalMesh != nullptr && mPose.GetMeshRevision() != mSkeletalMesh->GetRevision()) {
            ResolveMesh();
        }
    }

    /**
     * @brief 메시 경로를 에셋 관리자에서 해석합니다.
     */
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

    /**
     * @brief 현재 포즈와 재질을 복사한 프록시를 생성합니다.
     */
    Engine::FPrimitiveSceneProxy Engine::USkeletalMeshComponent::CreateSceneProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{Engine::UMeshComponent::CreateSceneProxy()};
        Proxy.mSkeletalMesh = mSkeletalMesh;

        // 렌더러는 프록시에 복사한 스키닝 행렬만 읽으며, 개정이 같으면 FScene::UploadSkinning이 업로드를 건너뜁니다.
        Proxy.mSkinMatrices = mPose.GetSkinMatrices();
        Proxy.mPoseRevision = mPose.GetRevision();

        return Proxy;
    }

    /**
     * @brief 저장 대상인 로컬 본 포즈를 반환합니다.
     */
    const Core::TArray<Core::FMatrix>& Engine::USkeletalMeshComponent::GetSerializedPose() const {
        return mPose.GetLocalTransforms();
    }

    // 전역 행렬과 피킹 형상은 복원 시 다시 계산하고 로컬 포즈만 저장합니다.

    /**
     * @brief 저장된 로컬 본 포즈를 검증하여 복원합니다.
     */
    bool Engine::USkeletalMeshComponent::SetSerializedPose(const Core::TArray<Core::FMatrix>& Transforms) {
        for (const Core::FMatrix& Transform : Transforms) {
            Core::FMatrix Inverse{};

            if (!Transform.IsFinite() || !Transform.TryInverse(Inverse) || !Inverse.IsFinite()) {
                return false;
            }
        }

        return mPose.SetLocalTransforms(Transforms);
    }

    /**
     * @brief 프로퍼티를 직렬화하고 로드한 포즈를 에셋에 다시 연결합니다.
     */
    void Engine::USkeletalMeshComponent::Serialize(Core::FArchive& Archive) {
        Engine::UMeshComponent::Serialize(Archive);

        if (Archive.IsLoading()) {
            if (!Archive.HasError() && mSkeletalMesh != nullptr) {
                ResolveMesh();
            }

            MarkRenderStateDirty();
        }
    }

}
