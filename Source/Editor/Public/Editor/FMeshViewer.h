/**
 * @file FMeshViewer.h
 * @brief 에디터와 독립 실행 모드에서 공유하는 정적·스켈레탈 메시 뷰어입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Input.h"
#include "Viewer/FAssetViewer.h"

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"
#include "Core/Guid.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FMatrix.h"
#include "Editor/FGizmoCore.h"
#include "Engine/EEditorMesh.h"
#include "Engine/FSceneView.h"
#include "Engine/EViewMode.h"
#include "Engine/FSkeletalMeshPose.h"

#include <filesystem>
#include <optional>

namespace Asset {
    class FAssetManager;
    class UStaticMesh;
    class USkeletalMesh;
    class USkeleton;
    class UMaterial;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Engine {
    class FSceneInterface;
    class FRenderTarget;
}

namespace Viewer {
    class FBoneShape;
    class FMeshViewerContent;
}

namespace Editor {

    /**
     * @brief 편집 월드와 분리한 렌더 장면, 카메라와 재질 미리보기를 소유합니다.
     */
    class FMeshViewer {
    public:

        /**
         * @brief 렌더러와 프로젝트 에셋 관리자를 연결합니다.
         * @param Renderer 렌더러의 비소유 주소입니다.
         * @param Assets 에셋 관리자의 비소유 주소입니다.
         */
        FMeshViewer(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets);

        /**
         * @brief 미리보기 장면과 렌더 타깃을 해제합니다.
         */
        ~FMeshViewer();

        /**
         * @brief GPU 미리보기 자원의 복사를 금지합니다.
         * @param Other 복사할 뷰어입니다.
         */
        FMeshViewer(const Editor::FMeshViewer& Other) = delete;

        /**
         * @brief GPU 미리보기 자원의 복사를 금지합니다.
         * @param Other 대입할 뷰어입니다.
         * @return 이 객체의 참조입니다.
         */
        Editor::FMeshViewer& operator=(const Editor::FMeshViewer& Other) = delete;

    public:

        /**
         * @brief 에셋을 독립 미리보기 장면에 엽니다.
         * @param Path Content 기준 또는 절대 메시 에셋 경로입니다.
         * @param Error 실패 설명을 받습니다.
         * @return 미리보기를 구성하면 true입니다.
         */
        bool Open(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 카메라·재질 조작 UI와 미리보기를 표시합니다.
         * @param Standalone 독립 실행용 전체 화면 창이면 true입니다.
         * @param[in] Input 키·마우스 상태를 조회할 공통 입력입니다.
         */
        void DrawUI(const Core::FInput& Input, bool Standalone = false);

        /**
         * @brief 창의 열림 상태를 검사합니다.
         * @return 뷰어가 열려 있으면 true입니다.
         */
        bool IsOpen() const;

        /**
         * @brief 열린 메시나 스켈레톤이 지정한 경로 아래에 있으면 뷰어를 닫습니다.
         * @details 에셋을 지우거나 옮겨 캐시에서 은퇴시킬 때 호출해, 옛 에셋을 계속 편집하지 않게 합니다.
         * @param[in] Path 은퇴시킨 에셋 파일 또는 폴더 경로입니다.
         */
        void CloseRetired(const std::filesystem::path& Path);

        /**
         * @brief 전체 메시가 보이도록 카메라를 배치합니다.
         */
        void FrameMesh();

    private:

        /**
         * @brief 현재 메시와 슬롯별 재질을 미리보기 장면에 반영합니다.
         */
        void UpdateScene();

        /**
         * @brief 슬롯별 임시 재질 선택 UI를 표시합니다.
         */
        void DrawMaterials();

        /**
         * @brief 스켈레탈 메시의 미리보기 스켈레톤 선택기를 표시합니다.
         */
        void DrawSkeletonPicker();

        /**
         * @brief 미리보기 옆에 본 계층 선택·기즈모 설정·포즈 복원 패널을 표시합니다.
         */
        void DrawPose();

        /**
         * @brief 본 하나와 그 자손을 계층 트리 노드로 표시하고 클릭하면 조작 대상으로 선택합니다.
         * @param Bone 표시할 본 번호입니다.
         * @param Children 본 번호별 자식 본 번호 목록입니다.
         */
        void DrawBoneNode(Core::Index_t Bone, const Core::TArray<Core::TArray<Core::Index_t>>& Children);

        /**
         * @brief 마우스 위치의 광선과 본 표시 도형을 비교하여 선택 본을 갱신합니다.
         * @param View 표시할 그래프 보기 또는 장면 뷰입니다.
         * @param Mouse 뷰포트 기준 마우스 좌표(픽셀)입니다.
         * @param Shapes 본 선택에 사용할 도형 목록입니다.
         * @return 선택할 본 도형과 교차했으면 true입니다.
         */
        bool PickBone(const Engine::FSceneView& View, const Core::FVector2& Mouse, const Core::TArray<Viewer::FBoneShape>& Shapes);

        /**
         * @brief 현재 궤도 카메라의 렌더링과 입력 해석에 쓸 뷰를 계산합니다.
         * @param Size 미리보기의 픽셀 크기입니다.
         * @return 계산한 카메라 뷰입니다.
         */
        Engine::FSceneView CalcSceneView(const Core::FVector2& Size) const;

        /**
         * @brief 선택한 본의 현재 전역 포즈로 기즈모 배치를 구성합니다.
         * @param Bone 월드 위치·회전을 계산할 스켈레톤 본 인덱스입니다.
         * @return 공통 코어에 전달할 기즈모 배치입니다.
         */
        Editor::FGizmoPlacement GetBonePlacement(Core::Index_t Bone) const;

        /**
         * @brief 기즈모의 호버와 드래그 입력을 처리합니다.
         * @param View 입력 해석에 사용할 카메라 뷰입니다.
         * @param Mouse 미리보기 기준 마우스 픽셀 좌표입니다.
         * @param Hovered 마우스가 미리보기 위에 있으면 true입니다.
         * @param Bone 입력으로 조작할 스켈레톤 본 인덱스입니다.
         * @return 기즈모가 마우스를 사용 중이어서 카메라 조작을 막아야 하면 true입니다.
         * @param[in] Input 키·마우스 상태를 조회할 공통 입력입니다.
         */
        bool UpdateBoneGizmo(Core::Index_t Bone, const Core::FInput& Input, const Engine::FSceneView& View, const Core::FVector2& Mouse, bool Hovered);

        /**
         * @brief 기즈모의 드래그 결과를 선택한 본의 부모 기준 로컬 변환으로 바꾸어 적용합니다.
         * @param Bone 기즈모 변환을 적용할 스켈레톤 본 인덱스입니다.
         */
        void ApplyBoneGizmo(Core::Index_t Bone);

        /**
         * @brief 카메라 입력을 처리하고 공통 렌더 경로로 메시와 본 도형을 그리며 아이콘·스프라이트·텍스트 표시는 제외합니다.
         * @param[in] Input 키·마우스 상태를 조회할 공통 입력입니다.
         */
        void DrawPreview(const Core::FInput& Input);

    private:

        /**
         * @brief 프로젝트 에셋 관리자의 비소유 주소입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 카메라·렌더 타깃·촬영을 담당하는 공통 에셋 뷰어입니다.
         */
        Viewer::FAssetViewer mViewer;

        /**
         * @brief 공통 뷰어가 소유하는 메시 콘텐츠의 비소유 포인터입니다.
         */
        Viewer::FMeshViewerContent* mContent{};

        /**
         * @brief 다음 표시에서 에셋 경계에 카메라를 맞출지 여부입니다.
         */
        bool mNeedsFit{true};

        /**
         * @brief 현재 메시의 비소유 주소입니다.
         */
        Asset::UStaticMesh* mMesh{};

        /**
         * @brief 스켈레탈 미리보기에서 사용하는 공유 에셋입니다.
         */
        Asset::USkeletalMesh* mSkeletalMesh{};

        /**
         * @brief 에셋 관리자가 소유하는 미리보기 스켈레톤입니다.
         */
        Asset::USkeleton* mSkeleton{};

        /**
         * @brief 스켈레톤 에셋 경로입니다.
         */
        Core::FString mSkeletonPath{};

        /**
         * @brief 본 조작에 사용하는 기즈모 표시와 드래그 계산입니다.
         */
        Editor::FGizmoCore mGizmo{};

        /**
         * @brief 본 기즈모의 변환 모드입니다.
         */
        Editor::EGizmoMode mGizmoMode{Editor::EGizmoMode::Rotate};

        /**
         * @brief 본 기즈모 축의 좌표계입니다.
         */
        Editor::EGizmoSpace mGizmoSpace{Editor::EGizmoSpace::Local};

        /**
         * @brief 드래그 시작 시 선택한 본의 전역 변환입니다.
         */
        Core::FMatrix mDragStartGlobal{};

        /**
         * @brief 드래그를 취소할 때 복원할 선택한 본의 로컬 변환입니다.
         */
        Core::FMatrix mDragStartLocal{};

        /**
         * @brief 기즈모 조작 대상으로 선택한 본입니다.
         */
        std::optional<Core::Index_t> mPoseBone{};

        /**
         * @brief 현재 본 계층을 보조 도형으로 표시할지 지정합니다.
         */
        bool mShowBones{true};

        /**
         * @brief 미리보기 슬롯별 오버라이드 경로입니다.
         */
        Core::TArray<Core::FString> mMaterialPaths{};

        /**
         * @brief 미리보기 에셋의 표시 경로입니다.
         */
        Core::FString mPath{};

        /**
         * @brief 최근 미리보기 실패의 설명입니다.
         */
        Core::FString mError{};

        /**
         * @brief 뷰어의 표시 모드입니다.
         */
        Engine::EViewMode mViewMode{Engine::EViewMode::Lit};

        /**
         * @brief 창의 열림 여부입니다.
         */
        bool mOpen{};

        /**
         * @brief 미리보기에서 시작한 마우스 드래그의 캡처 상태입니다.
         */
        bool mMouseCaptured{};
    };

}
