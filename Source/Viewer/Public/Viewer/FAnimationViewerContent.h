/**
 * @file FAnimationViewerContent.h
 * @brief 미리보기 월드에서 스켈레탈 메시와 단일 시퀀스를 재생하는 뷰어 콘텐츠입니다.
 * @ingroup Viewer
 */

#pragma once

#include "Viewer/IViewerContent.h"
#include "Viewer/FPreviewScene.h"
#include "Core/Math/FBox.h"

#include <memory>

namespace Asset {
    class FAssetManager;
    class UAnimSequence;
    class USkeletalMesh;
    class USkeleton;
}

namespace Engine {
    class FWorldContext;
    class UWorld;
    class USkeletalMeshComponent;
    class UAnimSingleNodeInstance;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Viewer {

    /**
     * @brief 미리보기 월드에서 스켈레탈 메시와 단일 시퀀스를 재생하는 뷰어 콘텐츠입니다.
     * @ingroup Viewer
     */
    class FAnimationViewerContent final : public Viewer::IViewerContent {
    public:

        /**
         * @brief 렌더러·에셋 서비스를 참조하는 빈 애니메이션 콘텐츠를 구성합니다.
         * @param Renderer 외부에서 소유하는 렌더러 서비스입니다.
         * @param Assets 외부에서 소유하는 에셋 관리자입니다.
         */
        FAnimationViewerContent(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FAnimationViewerContent() override;

    public:

        /**
         * @brief 호환되는 메시·스켈레톤으로 독립 미리보기 컴포넌트를 구성합니다.
         * @param Mesh 호출자가 수명을 보장하는 미리보기 메시입니다.
         * @param Skeleton 호출자가 수명을 보장하는 스켈레톤입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 연결에 성공하면 true이며 실패 시 기존 장면을 유지합니다.
         */
        bool SetMesh(Asset::USkeletalMesh* Mesh, Asset::USkeleton* Skeleton, Core::FString& Error);

        /**
         * @brief 외부 소유 시퀀스를 연결하고 단일 노드 재생 상태를 초기화합니다.
         * @param Sequence 시간 범위와 프레임률을 제공하는 시퀀스입니다.
         */
        void SetAnimation(const Asset::UAnimSequence* Sequence);

        /**
         * @brief 시퀀스를 재생하고 재생 방향의 끝에 있으면 시작 위치를 재설정합니다.
         */
        void Play();

        /**
         * @brief 현재 시각을 유지하면서 시퀀스 재생을 일시 정지합니다.
         */
        void Pause();

        /**
         * @brief 재생을 정지하고 지정한 시각의 포즈와 루트 모션 미리보기를 갱신합니다.
         * @param Time 시퀀스 또는 콘텐츠 시각(초)입니다.
         */
        void Seek(double Time);

        /**
         * @brief 재생 중인 컴포넌트를 갱신하고 루트 모션·포즈 경계를 장면에 반영합니다.
         * @param DeltaTime 이전 갱신 이후 경과 시간(초)입니다.
         */
        void Tick(float DeltaTime) override;

        /**
         * @brief 재생 시간을 진행하지 않고 현재 시각의 포즈와 장면 경계를 다시 계산합니다.
         */
        void RefreshPose();

        /**
         * @brief 애니메이션의 반복 재생 여부를 설정합니다.
         * @param Looping 끝점을 넘어 반복할지 여부입니다.
         */
        void SetLooping(bool Looping);

        /**
         * @brief 애니메이션 재생 속도 배율을 설정합니다.
         * @param PlayRate 원래 재생 속도에 곱하는 배율입니다.
         */
        void SetPlayRate(float PlayRate);

        /**
         * @brief 원본·제자리·루트 모션 적용 중 미리보기 방식을 변경합니다.
         * @param Mode 설정할 동작 또는 표시 모드입니다.
         */
        void SetPreviewMode(Core::Int32 Mode);

        /**
         * @brief 콘텐츠가 렌더링 또는 촬영 준비에 사용할 수 있는 상태인지 확인합니다.
         * @return 필요한 에셋과 표시 리소스가 준비되었으면 true입니다.
         */
        bool IsReady() const override;

        /**
         * @brief 시퀀스의 시간 진행 여부를 확인합니다.
         * @return 재생 중이면 true입니다.
         */
        bool IsPlaying() const;

        /**
         * @brief 시퀀스의 현재 재생 시각을 반환합니다.
         * @return 재생 시각(초)입니다.
         */
        double GetPosition() const;

        /**
         * @brief 애니메이션 재생 속도 배율을 반환합니다.
         * @return 원래 속도에 곱하는 재생 배율입니다.
         */
        float GetPlayRate() const;

        /**
         * @brief 애니메이션 반복 재생 설정을 반환합니다.
         * @return 반복 재생을 사용하면 true입니다.
         */
        bool IsLooping() const;

        /**
         * @brief 루트 모션 미리보기 방식을 반환합니다.
         * @return 0은 원본, 1은 제자리, 2는 루트 모션의 액터 적용입니다.
         */
        Core::Int32 GetPreviewMode() const;

        /**
         * @brief 현재 콘텐츠의 월드 공간 경계를 제공합니다.
         * @return 콘텐츠 수명에 종속되는 AABB의 읽기 전용 참조입니다.
         */
        const Core::FBox& GetBounds() const override;

        /**
         * @brief 독립 미리보기 월드를 반환합니다.
         * @return 월드 컨텍스트 소유의 비소유 포인터이며 초기화 전에는 nullptr입니다.
         */
        Engine::UWorld* GetWorld() const;

        /**
         * @brief 렌더링에 사용할 독립 미리보기 장면을 반환합니다.
         * @return 콘텐츠 또는 월드가 소유하는 비소유 장면 포인터이며 없으면 nullptr입니다.
         */
        Engine::FSceneInterface* GetScene() const override;

        /**
         * @brief 촬영 상태를 검증하고 지정 시각으로 이동하여 포즈를 준비합니다.
         * @param State 준비·저장·복원할 상태 데이터입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 포즈가 준비되면 Ready이며 실패하면 Failed와 오류를 반환합니다.
         */
        Viewer::EViewerPrepareResult PrepareCapture(const Viewer::FViewerCaptureState& State, Core::FString& Error) override;

        /**
         * @brief 미리보기 스켈레탈 메시 컴포넌트를 반환합니다.
         * @return 미리보기 월드 소유의 비소유 포인터이며 미설정 시 nullptr입니다.
         */
        Engine::USkeletalMeshComponent* GetComponent() const;

    private:

        /**
         * @brief 미리보기 컴포넌트의 단일 노드 애니메이션 인스턴스를 조회합니다.
         * @return 컴포넌트 소유의 비소유 포인터이며 없으면 nullptr입니다.
         */
        Engine::UAnimSingleNodeInstance* GetInstance() const;

        /**
         * @brief 미리보기 모드에 따라 액터 변환을 복원하거나 현재 시각까지의 루트 모션을 적용합니다.
         */
        void UpdateRootMotion();

        /**
         * @brief 현재 포즈의 월드 경계를 계산하고 프리미티브 변경을 렌더 장면에 반영합니다.
         */
        void SynchronizeScene();

    private:

        /**
         * @brief 외부에서 소유하는 렌더러 서비스입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 외부에서 소유하는 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 외부에서 수명을 보장하는 재생 시퀀스의 비소유 포인터입니다.
         */
        const Asset::UAnimSequence* mSequence{};

        /**
         * @brief 애니메이션 액터와 렌더 장면을 소유하는 독립 미리보기 월드입니다.
         */
        std::unique_ptr<Viewer::FPreviewScene> mScene{};

        /**
         * @brief 미리보기 월드가 소유하는 스켈레탈 메시 컴포넌트입니다.
         */
        Engine::USkeletalMeshComponent* mComponent{};

        /**
         * @brief 시퀀스 끝에서 반복 재생할지 여부입니다.
         */
        bool mLooping{true};

        /**
         * @brief 원래 재생 속도에 곱하는 배율이며 음수는 역재생입니다.
         */
        float mPlayRate{1.0f};

        /**
         * @brief 루트 모션 미리보기 방식이며 0은 원본, 1은 제자리, 2는 액터 이동입니다.
         */
        Core::Int32 mPreviewMode{};

        /**
         * @brief Fit과 컬링에 사용하는 미리보기 대상의 월드 공간 AABB입니다.
         */
        Core::FBox mBounds{};
    };

}
