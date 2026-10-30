/**
 * @file FMeshViewer.h
 * @brief 에디터와 독립 실행 모드에서 공유하는 정적 메시 뷰어입니다.
 * @ingroup Editor
 */
#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"
#include "Core/Guid.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FVector.h"
#include "Engine/EViewMode.h"

#include <filesystem>

class IRendererModule;
class FAssetManager;
class FSceneInterface;
class FRenderTarget;
class UStaticMesh;
class UMaterial;

/**
 * @brief 편집 월드와 분리한 렌더 장면, 카메라와 재질 미리보기를 소유합니다.
 */
class FMeshViewer {
public:
    /**
     * @brief 리플렉션 소유 타입의 별칭입니다.
     */
    ReflectTypes(FMeshViewer);

public:
    /**
     * @brief 렌더러와 프로젝트 에셋 관리자를 연결합니다.
     * @param Renderer 렌더러의 비소유 주소입니다.
     * @param Assets 에셋 관리자의 비소유 주소입니다.
     */
    FMeshViewer(IRendererModule* Renderer, FAssetManager* Assets);
    /**
     * @brief 미리보기 장면과 렌더 타깃을 해제합니다.
     */
    ~FMeshViewer();
    /**
     * @brief GPU 미리보기 자원의 복사를 금지합니다.
     * @param Other 복사할 뷰어입니다.
     */
    FMeshViewer(const FMeshViewer& Other) = delete;
    /**
     * @brief GPU 미리보기 자원의 복사를 금지합니다.
     * @param Other 대입할 뷰어입니다.
     * @return 이 객체의 참조입니다.
     */
    FMeshViewer& operator=(const FMeshViewer& Other) = delete;

public:
    /**
     * @brief 정적 타입 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터입니다.
     */
    ReflectStruct(FMeshViewer);
    /**
     * @brief 에셋을 독립 미리보기 장면에 엽니다.
     * @param Path Content 기준 또는 절대 메시 에셋 경로입니다.
     * @param Error 실패 설명을 받습니다.
     * @return 미리보기를 구성하면 true입니다.
     */
    bool Open(const std::filesystem::path& Path, FString& Error);
    /**
     * @brief 카메라·재질 조작 UI와 미리보기를 표시합니다.
     * @param Standalone 독립 실행용 전체 화면 창이면 true입니다.
     */
    void DrawUI(bool Standalone = false);
    /**
     * @brief 창의 열림 상태를 검사합니다.
     * @return 뷰어가 열려 있으면 true입니다.
     */
    bool IsOpen() const;
    /**
     * @brief 전체 메시가 보이도록 카메라를 배치합니다.
     */
    void FrameMesh();

private:
    /**
     * @brief 생성된 리플렉션 멤버 등록 함수의 접근을 허용합니다.
     */
    ReflectMembers();

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
     * @brief 카메라 입력을 처리하고 독립 렌더 타깃을 그립니다.
     */
    void DrawPreview();

private:
    /**
     * @brief 렌더러의 비소유 주소입니다.
     */
    IRendererModule* mRenderer{};
    ReflectProperty(mRenderer, "Renderer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 프로젝트 에셋 관리자의 비소유 주소입니다.
     */
    FAssetManager* mAssets{};
    ReflectProperty(mAssets, "Assets", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 편집 월드와 분리한 미리보기 장면입니다.
     */
    std::unique_ptr<FSceneInterface> mScene{};
    ReflectProperty(mScene, "Scene", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 미리보기 전용 렌더 타깃입니다.
     */
    std::unique_ptr<FRenderTarget> mTarget{};
    ReflectProperty(mTarget, "Target", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 현재 메시의 비소유 주소입니다.
     */
    UStaticMesh* mMesh{};
    ReflectProperty(mMesh, "Mesh", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 현재 슬롯별 미리보기 재질의 비소유 주소입니다.
     */
    TArray<UMaterial*> mMaterials{};
    ReflectProperty(mMaterials, "Materials", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 미리보기 슬롯별 오버라이드 경로입니다.
     */
    TArray<FString> mMaterialPaths{};
    ReflectProperty(mMaterialPaths, "MaterialPaths", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 미리보기 프리미티브의 식별자입니다.
     */
    FGuid mPrimitiveId{};
    ReflectProperty(mPrimitiveId, "PrimitiveId", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 미리보기 에셋의 표시 경로입니다.
     */
    FString mPath{};
    ReflectProperty(mPath, "Path", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 최근 미리보기 실패의 설명입니다.
     */
    FString mError{};
    ReflectProperty(mError, "Error", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 카메라 회전의 도 단위 값입니다.
     */
    FRotator mRotation{-20.0f, 45.0f, 0.0f};
    ReflectProperty(mRotation, "Rotation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 궤도 카메라의 중심입니다.
     */
    FVector mCenter{};
    ReflectProperty(mCenter, "Center", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 중심에서 카메라까지의 거리입니다.
     */
    float mDistance{10.0f};
    ReflectProperty(mDistance, "Distance", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 메시 경계 구의 반지름입니다.
     */
    float mRadius{1.0f};
    ReflectProperty(mRadius, "Radius", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 뷰어의 표시 모드입니다.
     */
    EViewMode mViewMode{EViewMode::Lit};
    ReflectProperty(mViewMode, "ViewMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 창의 열림 여부입니다.
     */
    bool mOpen{};
    ReflectProperty(mOpen, "Open", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 미리보기에서 시작한 마우스 드래그의 캡처 상태입니다.
     */
    bool mMouseCaptured{};
    ReflectProperty(mMouseCaptured, "MouseCaptured", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
