/**
 * @file USkeletalMeshComponent.cpp
 * @brief 스켈레탈 메시 참조와 객체별 포즈·재질·직렬화를 구현합니다.
 * @ingroup Engine
 */

#include "Engine/Component/USkeletalMeshComponent.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"
#include "Asset/FAssetManager.h"
#include "Core/Archive.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/UWorld.h"
#include "Engine/Animation/UAnimSingleNodeInstance.h"
#include "Engine/Animation/UAnimGraphInstance.h"
#include "Engine/Animation/FAnimNode_AssetPlayerBase.h"
#include "Engine/Animation/FAnimationRuntime.h"
#include "Engine/AActor.h"
#include "Asset/Animation/UAnimSequence.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/UClass.h"

#include <cmath>
#include <utility>

namespace Engine {

    /**
     * @brief 공유 바인드 형상을 객체별 포즈로 변형하여 표시하는 컴포넌트의 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief 저장 가능한 상대 에셋 경로 형식인지 검사합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @return 허용된 상대 경로이면 true입니다.
         */
        bool IsAssetPath(const Core::FString& Path) {
            const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};

            if (Path.find('\0') != Core::FString::npos || (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".masset"))) {
                return false;
            }

            for (const auto& Part : AssetPath) {
                if (Part == L"..") {
                    return false;
                }
            }

            return true;
        }
    }

    /**
     * @brief 에디터에서도 에셋 변경을 감지하는 컴포넌트를 생성합니다.
     */
    USkeletalMeshComponent::USkeletalMeshComponent() {
        SetEditorTickable(true);
    }

    /**
     * @brief 컴포넌트를 파괴합니다.
     */
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
        return mSkeletalMesh != nullptr && !mPose.GetRevision().IsValid() ? mSkeletalMesh->GetMeshData() : mPose.GetMeshData();
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
        if (!IsAssetPath(Path)) {
            return false;
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

    Asset::USkeleton* USkeletalMeshComponent::GetSkeleton() const {
        return mSkeleton;
    }

    const Core::FString& USkeletalMeshComponent::GetSkeletonPath() const {
        return mSkeletonPath;
    }

    bool USkeletalMeshComponent::SetSkeletonPath(const Core::FString& Path) {
        if (!IsAssetPath(Path)) {
            return false;
        }

        if (mSkeletonPath == Path && (Path.empty() || mSkeleton != nullptr)) {
            return true;
        }

        if (!Path.empty() && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr && LoadSkeleton(Path) == nullptr) {
            return false;
        }

        mSkeletonPath = Path;
        mSkeleton = nullptr;
        mPose = Engine::FSkeletalMeshPose{};

        ResolveMesh();
        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 저장된 에셋 경로를 해석합니다.
     */
    bool USkeletalMeshComponent::ResolveMesh() {
        Asset::USkeletalMesh* Mesh{LoadMesh(mMeshPath)};
        Asset::USkeleton* Skeleton{LoadSkeleton(mSkeletonPath)};

        // 캐시에 올라간 에셋은 내용이 바뀌지 않으므로 객체가 같으면 다시 해석할 필요가 없습니다.
        if (Mesh != mSkeletalMesh || Skeleton != mSkeleton) {
            mSkeletalMesh = Mesh;
            mSkeleton = Skeleton;
            mPose.Invalidate();

            if (Mesh != nullptr && Skeleton != nullptr) {
                bool PoseReady{false};

                if (!mPose.GetLocalTransforms().empty()) {
                    PoseReady = mPose.Recalculate(*Mesh, *Skeleton);

                    if (!PoseReady) {
                        Core::ErrorHandler::Report("SkeletalMesh", "Stored pose does not match the skeleton; using bind pose", Core::ErrorHandler::EErrorLevel::Warning);
                    }
                }

                if (!PoseReady && !mPose.Reset(*Mesh, *Skeleton)) {
                    mPose = Engine::FSkeletalMeshPose{};
                }
            }

            // 애니메이션 인스턴스의 트랙 → 본 매핑은 스켈레톤 본 순서에 묶여 있으므로, 메시·스켈레톤이 바뀌면 다시 초기화해 매핑을 비웁니다.
            if (mAnimInstance != nullptr) {
                mAnimInstance->InitializeAnimation(this);
            }

            MarkRenderStateDirty();
        }

        return (mMeshPath.empty() || mSkeletalMesh != nullptr) && (mSkeletonPath.empty() || mSkeleton != nullptr);
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
        if (mSkeletalMesh == nullptr || mSkeleton == nullptr || !mPose.SetLocalTransform(*mSkeletalMesh, *mSkeleton, Bone, Transform)) {
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
        const bool Updated{mSkeletalMesh != nullptr && mSkeleton != nullptr ? mPose.SetLocalTransforms(*mSkeletalMesh, *mSkeleton, Transforms) : mPose.SetLocalTransforms(Transforms)};

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
        if (mSkeletalMesh == nullptr || mSkeleton == nullptr || !mPose.Reset(*mSkeletalMesh, *mSkeleton)) {
            return false;
        }

        MarkRenderStateDirty();

        return true;
    }

    /**
     * @brief 에셋과 포즈가 준비되었는지 검사합니다.
     */
    bool USkeletalMeshComponent::ShouldCreateRenderState() const {
        return mSkeletalMesh != nullptr;
    }

    /**
     * @brief 재등록 때 재해석하도록 에셋 참조를 해제합니다.
     */
    void USkeletalMeshComponent::UnregisterComponent() {
        Engine::UWorld::FDispatchScope Scope{GetWorld()};

        Engine::UMeshComponent::UnregisterComponent();

        mSkeletalMesh = nullptr;
        mSkeleton = nullptr;
        mPose.Invalidate();
    }

    /**
     * @brief 재생 중인 애니메이션을 진행합니다.
     */
    void USkeletalMeshComponent::Tick(float DeltaTime) {
        Engine::UMeshComponent::Tick(DeltaTime);

        // 에디터 월드는 BeginPlay를 받지 않으므로 애니메이션은 PIE와 게임에서만 진행됩니다.
        if (HasBegunPlay()) {
            TickAnimation(DeltaTime);
        }
    }

    void USkeletalMeshComponent::BeginPlay() {
        Engine::UMeshComponent::BeginPlay();

        // AnimClass가 있으면 그 그래프를 처음 상태로 초기화해 재생하고, 없으면 Animation 시퀀스를 단일 노드로 재생합니다.
        if (!mAnimClass.empty()) {
            if (mAnimInstance == nullptr) {
                CreateAnimClassInstance();
            }

            InitializeAnimClassInstance();
        } else if (!mAnimationPath.empty()) {
            if (const Asset::UAnimSequence* Sequence{LoadAnimation(mAnimationPath)}) {
                PlayAnimation(Sequence, mLooping);
            }
        }
    }

    void USkeletalMeshComponent::EndPlay() {
        if (mAnimInstance != nullptr) {
            mAnimInstance->ResetRootMotion();
        }

        // AnimClass 인스턴스는 설정 값을 담고 있으므로 남겨 둡니다.
        if (mAnimClass.empty()) {
            mAnimInstance.reset();
        }

        Engine::UMeshComponent::EndPlay();
    }

    const Core::FString& USkeletalMeshComponent::GetAnimClass() const {
        return mAnimClass;
    }

    bool USkeletalMeshComponent::SetAnimClass(const Core::FString& ClassName) {
        if (!ClassName.empty()) {
            const CoreUObject::UClass* Class{CoreUObject::FReflectionRegistry::FindClass(ClassName)};

            if (Class == nullptr || !Class->IsChildOf(Engine::UAnimInstance::StaticClass()) || !Class->CanCreate()) {
                return false;
            }
        }

        if (mAnimClass == ClassName && (ClassName.empty() || (mAnimInstance != nullptr && mAnimInstance->GetClass()->GetName() == ClassName))) {
            return true;
        }

        if (const auto* Graph{CoreUObject::Cast<Engine::UAnimGraphInstance>(mAnimInstance.get())}) {
            mInactiveAnimGraph = Graph->GetAnimGraph();
        }

        mAnimClass = ClassName;
        mAnimInstance.reset();

        if (!ClassName.empty()) {
            CreateAnimClassInstance();
            InitializeAnimClassInstance();
        } else if (HasBegunPlay() && !mAnimationPath.empty()) {
            PlayAnimation(LoadAnimation(mAnimationPath), mLooping);
        }

        return true;
    }

    bool USkeletalMeshComponent::CreateAnimClassInstance() {
        const CoreUObject::UClass* Class{mAnimClass.empty() ? nullptr : CoreUObject::FReflectionRegistry::FindClass(mAnimClass)};

        if (Class == nullptr || !Class->IsChildOf(Engine::UAnimInstance::StaticClass())) {
            return false;
        }

        std::unique_ptr<CoreUObject::UObject> Object{Class->CreateInstance()};
        auto* Instance{CoreUObject::Cast<Engine::UAnimInstance>(Object.get())};

        if (Instance == nullptr) {
            return false;
        }

        Object.release();
        mAnimInstance.reset(Instance);
        mAnimInstance->SetName("AnimInstance");
        mAnimInstance->SetOuter(this);

        if (auto* Graph{CoreUObject::Cast<Engine::UAnimGraphInstance>(Instance)}; Graph != nullptr && !mInactiveAnimGraph.empty()) {
            Graph->SetAnimGraph(mInactiveAnimGraph);
            mInactiveAnimGraph.clear();
        }

        return true;
    }

    void USkeletalMeshComponent::InitializeAnimClassInstance() {
        // 에셋은 월드의 에셋 관리자로 로드하므로, 월드에 등록되기 전이면 ResolveMesh가 메시를 해석할 때 초기화합니다.
        if (mAnimInstance != nullptr && GetWorld() != nullptr) {
            mAnimInstance->InitializeAnimation(this);
        }
    }

    void USkeletalMeshComponent::PlayAnimation(const Asset::UAnimSequence* Sequence, bool Looping) {
        auto* SingleNode{CoreUObject::Cast<Engine::UAnimSingleNodeInstance>(mAnimInstance.get())};

        // 언리얼의 AnimationSingleNode 모드와 같이 단일 노드 인스턴스가 아니면 새로 만듭니다.
        if (SingleNode == nullptr) {
            auto Instance{CoreUObject::NewObject<Engine::UAnimSingleNodeInstance>()};

            SingleNode = Instance.get();
            mAnimInstance = std::move(Instance);
            mAnimInstance->InitializeAnimation(this);
            SingleNode->SetRootMotionMode(mSingleNodeRootMotionMode);
        }

        mLooping = Looping;
        SingleNode->SetAnimation(Sequence);
        SingleNode->SetPlayRate(mPlayRate);
        SingleNode->Play(Looping);
    }

    Engine::UAnimInstance* USkeletalMeshComponent::GetAnimInstance() const {
        return mAnimInstance.get();
    }

    bool USkeletalMeshComponent::SetAnimGraphFloat(const Core::FString& Name, float Value) {
        auto* Instance{CoreUObject::Cast<Engine::UAnimGraphInstance>(mAnimInstance.get())};

        return Instance != nullptr && Instance->SetFloatVariable(Name, Value);
    }

    bool USkeletalMeshComponent::SetAnimGraphBool(const Core::FString& Name, bool Value) {
        auto* Instance{CoreUObject::Cast<Engine::UAnimGraphInstance>(mAnimInstance.get())};

        return Instance != nullptr && Instance->SetBoolVariable(Name, Value);
    }

    bool USkeletalMeshComponent::RestartAnimGraphPlayer(const Core::FString& Name) {
        auto* Instance{CoreUObject::Cast<Engine::UAnimGraphInstance>(mAnimInstance.get())};
        auto* Player{Instance != nullptr ? Instance->FindAssetPlayer(Name) : nullptr};

        if (Player == nullptr) {
            return false;
        }

        Player->Initialize(*Instance);
        Instance->ResetRootMotion();

        return true;
    }

    bool USkeletalMeshComponent::IsAnimGraphPlayerFinished(const Core::FString& Name) const {
        const auto* Instance{CoreUObject::Cast<Engine::UAnimGraphInstance>(mAnimInstance.get())};
        const auto* Player{Instance != nullptr ? Instance->FindAssetPlayer(Name) : nullptr};

        return Player != nullptr && Player->IsFinished();
    }

    const Core::FString& USkeletalMeshComponent::GetAnimationPath() const {
        return mAnimationPath;
    }

    bool USkeletalMeshComponent::SetAnimationPath(const Core::FString& Path) {
        if (!IsAssetPath(Path)) {
            return false;
        }

        mAnimationPath = Path;

        // 플레이 중에 바꾸면 새 애니메이션을 바로 재생하고, 비우면 레퍼런스 포즈를 출력합니다.
        if (HasBegunPlay()) {
            PlayAnimation(Path.empty() ? nullptr : LoadAnimation(Path), mLooping);
        }

        return true;
    }

    bool USkeletalMeshComponent::IsLooping() const {
        return mLooping;
    }

    void USkeletalMeshComponent::SetLooping(bool Looping) {
        mLooping = Looping;

        if (auto* SingleNode{CoreUObject::Cast<Engine::UAnimSingleNodeInstance>(mAnimInstance.get())}) {
            SingleNode->SetLooping(Looping);
        }
    }

    float USkeletalMeshComponent::GetPlayRate() const {
        return mPlayRate;
    }

    void USkeletalMeshComponent::SetPlayRate(float PlayRate) {
        mPlayRate = std::isfinite(PlayRate) ? PlayRate : 1.0f;

        if (auto* SingleNode{CoreUObject::Cast<Engine::UAnimSingleNodeInstance>(mAnimInstance.get())}) {
            SingleNode->SetPlayRate(mPlayRate);
        }
    }

    void USkeletalMeshComponent::TickAnimation(float DeltaTime) {
        if (mAnimInstance == nullptr || mSkeletalMesh == nullptr || mSkeleton == nullptr) {
            return;
        }

        mAnimInstance->UpdateAnimation(DeltaTime);

        const Core::TArray<Core::FTransform>& Pose{mAnimInstance->GetPose()};
        Core::TArray<Core::FMatrix> Locals{};

        Locals.reserve(Pose.size());

        for (const Core::FTransform& Transform : Pose) {
            Locals.push_back(Transform.ToMatrix());
        }

        // 스키닝 행렬만 다시 계산하고 피킹 형상과 경계는 갱신하지 않습니다. 프록시를 다시 만들어 새 스키닝 행렬을 렌더러에 넘깁니다.
        if (mPose.SetAnimatedLocalTransforms(*mSkeletalMesh, *mSkeleton, std::move(Locals))) {
            MarkRenderStateDirty();

            if (mApplyRootMotionToActor) {
                ApplyRootMotion(mAnimInstance->ConsumeExtractedRootMotion());
            }
        } else {
            mAnimInstance->ResetRootMotion();
        }
    }

    Engine::ERootMotionMode USkeletalMeshComponent::GetSingleNodeRootMotionMode() const {
        return mSingleNodeRootMotionMode;
    }

    void USkeletalMeshComponent::SetSingleNodeRootMotionMode(Engine::ERootMotionMode Mode) {
        if (Mode < Engine::ERootMotionMode::NoRootMotionExtraction || Mode > Engine::ERootMotionMode::RootMotionFromEverything) {
            return;
        }

        mSingleNodeRootMotionMode = Mode;

        if (auto* SingleNode{CoreUObject::Cast<Engine::UAnimSingleNodeInstance>(mAnimInstance.get())}) {
            SingleNode->SetRootMotionMode(Mode);
        }
    }

    bool USkeletalMeshComponent::IsRootMotionAppliedToActor() const {
        return mApplyRootMotionToActor;
    }

    Engine::ERootMotionMode USkeletalMeshComponent::GetRootMotionMode() const {
        return mAnimInstance != nullptr ? mAnimInstance->GetRootMotionMode() : mSingleNodeRootMotionMode;
    }

    void USkeletalMeshComponent::SetRootMotionMode(Engine::ERootMotionMode Mode) {
        if (mAnimInstance != nullptr) {
            mAnimInstance->SetRootMotionMode(Mode);
        }

        if (mAnimInstance == nullptr || CoreUObject::Cast<Engine::UAnimSingleNodeInstance>(mAnimInstance.get()) != nullptr) {
            SetSingleNodeRootMotionMode(Mode);
        }
    }

    Core::Int32 USkeletalMeshComponent::GetRootMotionModeValue() const {
        return static_cast<Core::Int32>(GetRootMotionMode());
    }

    bool USkeletalMeshComponent::SetRootMotionModeValue(Core::Int32 Mode) {
        if (Mode < static_cast<Core::Int32>(Engine::ERootMotionMode::NoRootMotionExtraction) || Mode > static_cast<Core::Int32>(Engine::ERootMotionMode::RootMotionFromEverything)) {
            return false;
        }

        SetRootMotionMode(static_cast<Engine::ERootMotionMode>(Mode));

        return true;
    }

    void USkeletalMeshComponent::SetApplyRootMotionToActor(bool Apply) {
        if (mApplyRootMotionToActor != Apply) {
            mApplyRootMotionToActor = Apply;

            if (mAnimInstance != nullptr) {
                mAnimInstance->ResetRootMotion();
            }
        }
    }

    Core::FTransform USkeletalMeshComponent::ConsumeRootMotion() {
        return mAnimInstance != nullptr ? mAnimInstance->ConsumeExtractedRootMotion() : Core::FTransform{};
    }

    void USkeletalMeshComponent::ApplyRootMotion(const Core::FTransform& Delta) {
        Engine::AActor* Owner{GetOwner()};

        if (Owner == nullptr || (Delta.mLocation.LengthSquared() <= 1.e-12f && Delta.mRotation.Equals(Core::FQuat{}))) {
            return;
        }

        Owner->SetActorTransform(Engine::FAnimationRuntime::ApplyRootMotionToTransform(Delta, GetTransform(), Owner->GetActorTransform()));
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

    Asset::USkeleton* USkeletalMeshComponent::LoadSkeleton(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr) {
            return nullptr;
        }

        if (Path.empty()) {
            return mMeshPath.empty() ? nullptr : Assets->FindLegacySkeleton(Core::FilePath::FromUtf8(mMeshPath));
        }

        Core::FString Error{};
        Asset::USkeleton* Skeleton{Assets->LoadSkeleton(Core::FilePath::FromUtf8(Path), Error)};

        if (Skeleton == nullptr) {
            Core::ErrorHandler::Report("Skeleton", "Cannot load skeleton '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Skeleton;
    }

    Asset::UAnimSequence* USkeletalMeshComponent::LoadAnimation(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Asset::UAnimSequence* Sequence{Assets->LoadAnimSequence(Core::FilePath::FromUtf8(Path), Error)};

        if (Sequence == nullptr) {
            Core::ErrorHandler::Report("Animation", "Cannot load animation '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Sequence;
    }

    /**
     * @brief 현재 포즈와 재질을 복사한 프록시를 생성합니다.
     */
    Engine::FPrimitiveSceneProxy Engine::USkeletalMeshComponent::CreateSceneProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{Engine::UMeshComponent::CreateSceneProxy()};

        if (mPose.GetRevision().IsValid()) {
            // 렌더러는 프록시에 복사한 스키닝 행렬만 읽으며, 개정이 같으면 FScene::UploadSkinning이 업로드를 건너뜁니다.
            Proxy.mSkeletalMesh = mSkeletalMesh;
            Proxy.mSkinMatrices = mPose.GetSkinMatrices();
            Proxy.mPoseRevision = mPose.GetRevision();
        }

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
        if (Archive.IsLoading()) {
            mInactiveAnimGraph.clear();
        }

        Engine::UMeshComponent::Serialize(Archive);

        if (!Archive.HasError() && (!Archive.IsLoading() || Archive.HasField("InactiveAnimGraph"))) {
            Archive.Serialize("InactiveAnimGraph", mInactiveAnimGraph);

            if (Archive.IsLoading() && !IsAssetPath(mInactiveAnimGraph)) {
                Archive.SetError("Invalid inactive animation graph path");
            }
        }

        // AnimClass 인스턴스의 설정 값은 AnimInstance 객체 안에 저장합니다. 식별자와 이름 같은 UObject 공통 프로퍼티는 저장하지 않습니다.
        // 읽을 때는 AnimClass setter가 이미 인스턴스를 만들었습니다.
        Engine::UAnimInstance* Instance{!mAnimClass.empty() && mAnimInstance != nullptr && mAnimInstance->GetClass()->GetName() == mAnimClass ? mAnimInstance.get() : nullptr};

        if (Instance != nullptr && !Archive.HasError() && (!Archive.IsLoading() || Archive.HasField("AnimInstance")) && Archive.BeginObject("AnimInstance")) {
            for (const bool Late : {false, true}) {
                for (const CoreUObject::UClass* Class{Instance->GetClass()}; Class != nullptr && Class->IsChildOf(Engine::UAnimInstance::StaticClass()); Class = Class->GetSuperClass()) {
                    for (const CoreUObject::FProperty* Property : Class->GetProperties()) {
                        if (Property->HasFlag(Core::EPropertyFlags::Serialize) && Property->HasFlag(Core::EPropertyFlags::SerializeLate) == Late) {
                            Property->SerializeValue(Instance, Archive);
                        }
                    }
                }
            }

            Archive.EndObject();

            // 읽은 설정으로 그래프를 다시 만듭니다. 메시가 없으면 아래 ResolveMesh가 다시 초기화하지 않습니다.
            if (Archive.IsLoading() && !Archive.HasError()) {
                InitializeAnimClassInstance();
            }
        }

        if (Archive.IsLoading()) {
            if (!Archive.HasError()) {
                // 경로가 같아도 읽어 온 포즈를 다시 적용해야 하므로 에셋 참조를 비워 강제로 다시 해석합니다.
                mSkeletalMesh = nullptr;
                mSkeleton = nullptr;
                ResolveMesh();
            }

            MarkRenderStateDirty();
        }
    }

}
