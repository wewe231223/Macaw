/**
 * @file USkeletalMeshComponent.h
 * @brief 스켈레탈 메시의 에셋·재질과 객체별 본 포즈를 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UMeshComponent.h"
#include "Engine/FSkeletalMeshPose.h"

#include <memory>
#include "USkeletalMeshComponent.generated.h"

namespace Asset {
    class UAnimSequence;
    class USkeletalMesh;
    class USkeleton;
}

namespace Engine {

    class UAnimInstance;
    class UAnimSingleNodeInstance;

    /**
     * @brief 공유 바인드 형상을 객체별 포즈로 변형하여 표시하는 컴포넌트입니다.
     */
    UCLASS()
    class USkeletalMeshComponent final : public Engine::UMeshComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 에디터에서도 에셋 변경을 감지하는 컴포넌트를 생성합니다.
         */
        USkeletalMeshComponent();

        /**
         * @brief 컴포넌트를 파괴합니다.
         */
        ~USkeletalMeshComponent() override;

    public:
        /**
         * @brief 기본 프리미티브 구분값을 반환합니다.
         * @return 에셋이 없을 때도 대신 표시하지 않는 Cube 구분값입니다.
         */
        Engine::EPrimitiveType GetPrimitiveType() const override;

        /**
         * @brief 공유 바인드 형상을 반환합니다.
         * @return 공통 렌더 경로용 형상이며 없으면 nullptr입니다.
         */
        Asset::UStaticMesh* GetStaticMesh() const override;

        /**
         * @brief 연결된 스켈레탈 메시를 반환합니다.
         * @return 에셋 관리자가 소유하는 비소유 주소입니다.
         */
        Asset::USkeletalMesh* GetSkeletalMesh() const;

        /**
         * @brief 현재 변형 위치와 경계를 가진 CPU 형상을 반환합니다.
         * @return 피킹용 형상입니다.
         */
        const Asset::FMeshData& GetMeshData() const override;

        /**
         * @brief 프로젝트 상대 에셋 경로를 반환합니다.
         * @return 저장되는 메시 경로입니다.
         */
        const Core::FString& GetMeshPath() const;

        /**
         * @brief 에셋 경로를 설정하고 포즈를 초기화합니다.
         * @param Path Content 기준 .masset 경로입니다.
         * @return 경로가 유효하고 가능한 경우 로드했으면 true입니다.
         */
        bool SetMeshPath(const Core::FString& Path);

        Asset::USkeleton* GetSkeleton() const;
        const Core::FString& GetSkeletonPath() const;
        bool SetSkeletonPath(const Core::FString& Path);

        /**
         * @brief 저장된 에셋 경로와 현재 에셋 개정을 해석합니다.
         * @return 에셋과 포즈를 준비했으면 true입니다.
         */
        bool ResolveMesh() override;

        /**
         * @brief 현재 본 포즈를 반환합니다.
         * @return 컴포넌트가 소유한 포즈입니다.
         */
        const Engine::FSkeletalMeshPose& GetPose() const;

        /**
         * @brief 본 하나의 부모 기준 변환을 변경합니다.
         * @param Bone 본 인덱스입니다.
         * @param Transform 부모 기준 아핀 행렬입니다.
         * @return 유효한 포즈를 적용하면 true입니다.
         */
        bool SetBoneLocalTransform(Core::Uint32 Bone, const Core::FMatrix& Transform);

        /**
         * @brief 전체 로컬 포즈를 변경합니다.
         * @param Transforms 본 인덱스순 부모 기준 행렬입니다.
         * @return 적용하면 true입니다.
         */
        bool SetLocalPose(const Core::TArray<Core::FMatrix>& Transforms);

        /**
         * @brief 모든 본을 바인드 포즈로 복원합니다.
         * @return 복원하면 true입니다.
         */
        bool ResetPose();

        /**
         * @brief 저장 대상인 로컬 본 포즈를 반환합니다.
         * @return 포즈가 유지하는 로컬 행렬의 참조입니다.
         */
        const Core::TArray<Core::FMatrix>& GetSerializedPose() const;

        /**
         * @brief 저장된 로컬 본 포즈를 검증하여 복원합니다.
         * @param Transforms 복원할 로컬 행렬입니다.
         * @return 모든 행렬이 유한하고 역행렬을 가지면 true입니다.
         */
        bool SetSerializedPose(const Core::TArray<Core::FMatrix>& Transforms);

        /**
         * @brief 현재 포즈와 재질을 복사한 프록시를 생성합니다.
         * @return GPU 스키닝에 필요한 프록시입니다.
         */
        Engine::FPrimitiveSceneProxy CreateSceneProxy() const override;

        /**
         * @brief 에셋과 포즈가 준비되었는지 검사합니다.
         * @return 표시 가능한 경우 true입니다.
         */
        bool ShouldCreateRenderState() const override;

        /**
         * @brief 재등록 때 재해석하도록 에셋 참조를 해제합니다.
         */
        void UnregisterComponent() override;

        /**
         * @brief 플레이 중이면 애니메이션 포즈를 갱신합니다.
         * @param DeltaTime 프레임 경과 시간입니다.
         */
        void Tick(float DeltaTime) override;

        /**
         * @brief 지정한 애니메이션 에셋이 있으면 재생을 시작합니다.
         */
        void BeginPlay() override;

        /**
         * @brief 애니메이션 인스턴스를 해제하고 마지막 포즈를 유지합니다.
         */
        void EndPlay() override;

        /**
         * @brief 단일 노드 애니메이션 인스턴스로 시퀀스 하나를 재생합니다.
         * @details 언리얼 PlayAnimation과 같이 AnimationSingleNode 모드로 바꾸고 시퀀스를 지정한 뒤 재생을 시작합니다. 포즈는 플레이 중 Tick에서만 갱신됩니다.
         * @param[in] Sequence 재생할 시퀀스입니다. 에셋 관리자가 소유합니다.
         * @param[in] Looping 반복 재생 여부입니다.
         */
        void PlayAnimation(const Asset::UAnimSequence* Sequence, bool Looping);

        /**
         * @brief 현재 애니메이션 인스턴스를 반환합니다.
         * @details AnimClass를 지정했으면 에디터에서도 항상 그 클래스의 인스턴스가 있으며, 설정 값을 담고 시각화 도구가 그래프를 표시합니다.
         * AnimClass가 비어 있으면 플레이 중 단일 노드 인스턴스만 있습니다.
         * @return 없으면 nullptr입니다.
         */
        Engine::UAnimInstance* GetAnimInstance() const;

        UFUNCTION()
        bool SetAnimGraphFloat(const Core::FString& Name, float Value);

        UFUNCTION()
        bool SetAnimGraphBool(const Core::FString& Name, bool Value);

        UFUNCTION()
        bool RestartAnimGraphPlayer(const Core::FString& Name);

        UFUNCTION()
        bool IsAnimGraphPlayerFinished(const Core::FString& Name) const;

        /**
         * @brief 애니메이션 인스턴스 클래스 이름을 반환합니다.
         * @return UAnimInstance 파생 클래스 이름이며, 비어 있으면 Animation 시퀀스를 단일 노드로 재생합니다.
         */
        const Core::FString& GetAnimClass() const;

        /**
         * @brief 애니메이션 인스턴스 클래스를 바꾸고 그 클래스의 인스턴스를 새로 만듭니다.
         * @details 언리얼의 AnimClass(애니메이션 블루프린트)에 대응합니다. 클래스를 바꾸면 이전 인스턴스의 설정 값은 버립니다.
         * @param[in] ClassName 생성할 수 있는 UAnimInstance 파생 클래스 이름이거나 빈 문자열입니다.
         * @return 클래스가 유효해 적용했으면 true입니다.
         */
        bool SetAnimClass(const Core::FString& ClassName);

        const Core::FString& GetAnimationPath() const;
        bool SetAnimationPath(const Core::FString& Path);

        bool IsLooping() const;
        void SetLooping(bool Looping);

        float GetPlayRate() const;
        void SetPlayRate(float PlayRate);

        /**
         * @brief 프로퍼티를 직렬화하고 로드한 포즈를 에셋에 다시 연결합니다.
         * @param Archive 저장 또는 복원할 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive) override;

    private:
        /**
         * @brief 메시 경로를 에셋 관리자에서 해석합니다.
         * @param Path Content 상대 경로입니다.
         * @return 로드 실패 시 nullptr입니다.
         */
        Asset::USkeletalMesh* LoadMesh(const Core::FString& Path) const;
        Asset::USkeleton* LoadSkeleton(const Core::FString& Path) const;
        Asset::UAnimSequence* LoadAnimation(const Core::FString& Path) const;

        /**
         * @brief AnimClass의 인스턴스를 새로 만들고 Outer를 이 컴포넌트로 지정합니다.
         * @return AnimClass가 비어 있거나 생성할 수 없으면 false입니다.
         */
        bool CreateAnimClassInstance();

        /**
         * @brief AnimClass 인스턴스를 초기화합니다. 월드가 없으면 에셋을 로드할 수 없으므로 등록 뒤 ResolveMesh가 다시 호출합니다.
         */
        void InitializeAnimClassInstance();

        /**
         * @brief 애니메이션 인스턴스를 갱신하고 결과 포즈를 스키닝 행렬에 반영합니다.
         * @param DeltaTime 프레임 경과 시간입니다.
         */
        void TickAnimation(float DeltaTime);

    private:
        /**
         * @brief Content 기준 스켈레탈 메시 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "SkeletalMesh", Getter = GetMeshPath, Setter = SetMeshPath)
        Core::FString mMeshPath{};

        UPROPERTY(Edit, Serialize, Name = "Skeleton", Getter = GetSkeletonPath, Setter = SetSkeletonPath)
        Core::FString mSkeletonPath{};

        /**
         * @brief 애니메이션 그래프를 만드는 UAnimInstance 파생 클래스 이름입니다. 비어 있으면 Animation을 단일 노드로 재생합니다.
         * @details 인스턴스의 Serialize 프로퍼티는 이 컴포넌트를 저장할 때 AnimInstance 객체 안에 함께 저장합니다.
         */
        UPROPERTY(Edit, Serialize, Name = "AnimClass", Getter = GetAnimClass, Setter = SetAnimClass)
        Core::FString mAnimClass{};
        Core::FString mInactiveAnimGraph{};

        /**
         * @brief 플레이를 시작하면 재생할 Content 기준 애니메이션 시퀀스 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "Animation", Getter = GetAnimationPath, Setter = SetAnimationPath)
        Core::FString mAnimationPath{};

        /**
         * @brief 애니메이션을 반복 재생할지 여부입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "Looping", Getter = IsLooping, Setter = SetLooping)
        bool mLooping{true};

        /**
         * @brief 애니메이션 재생 속도 배율입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "PlayRate", Getter = GetPlayRate, Setter = SetPlayRate)
        float mPlayRate{1.0f};

        /**
         * @brief 애니메이션 인스턴스입니다. AnimClass가 있으면 항상 존재하고, 없으면 플레이 중 단일 노드 인스턴스만 존재합니다.
         * @details Outer는 이 컴포넌트이므로 에디터는 인스턴스를 월드 객체로 편집할 수 있습니다.
         */
        std::unique_ptr<Engine::UAnimInstance> mAnimInstance{};

        /**
         * @brief 에셋 관리자가 소유하는 메시입니다.
         */
        Asset::USkeletalMesh* mSkeletalMesh{};
        Asset::USkeleton* mSkeleton{};

        /**
         * @brief 객체별 로컬·전역 포즈와 CPU 피킹 형상입니다.
         */
        UPROPERTY(Serialize, SerializeLate, Name = "BoneLocalTransforms", Getter = GetSerializedPose, Setter = SetSerializedPose, Array)
        Engine::FSkeletalMeshPose mPose{};

        // 메시 setter가 포즈를 초기화하므로 메시 경로를 복원한 뒤에 로드합니다.
    };

}
