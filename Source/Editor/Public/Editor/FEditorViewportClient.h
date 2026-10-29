/**
 * @file FEditorViewportClient.h
 * @brief 에디터 카메라, 선택, 공통 명령과 기즈모 입력을 처리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FViewportClient.h"
#include "Editor/Gizmo.h"
#include "Editor/EViewportCommand.h"
#include "Editor/EEditorViewportType.h"
#include "Engine/EViewMode.h"
#include "Engine/FEngineShowFlags.h"
#include "Engine/FEditorCameraState.h"

#include <array>
#include <filesystem>

class FEditorContext;
class UPrimitiveComponent;
struct FBox;

/**
 * @brief 에디터 카메라, 선택, 변환 도구 상태와 공통 명령을 관리하고 기즈모 입력 및 장면 뷰 생성을 담당합니다.
 * @ingroup Editor
 * @note 툴바, 메뉴와 단축키는 동일한 명령 경로를 사용하며 기즈모는 이 클라이언트의 모드와 좌표계를 읽습니다.
 */
class FEditorViewportClient final : public FViewportClient {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FEditorViewportClient, FViewportClient);

public:
    /**
     * @brief FEditorViewportClient의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in,out] Context 패널 또는 뷰포트가 공유할 편집 컨텍스트입니다.
     */
    explicit FEditorViewportClient(FEditorContext* Context);
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FEditorViewportClient() override;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FEditorViewportClient);
    /** @brief 기즈모의 진행 중인 변환을 취소하고 공유할 편집 컨텍스트를 변경합니다. @param[in] Context 새 비소유 컨텍스트이며 nullptr이면 기존 연결을 유지합니다. */
    void SetEditorContext(FEditorContext* Context);

    /**
     * @brief 실행 가능 여부를 검사한 뒤 뷰포트 명령을 처리합니다.
     * @param[in] Command 툴바, 메뉴 또는 단축키에서 요청한 동작입니다.
     * @return 명령을 처리하면 true이며 차단되거나 지원하지 않는 명령이면 상태를 변경하지 않고 false입니다.
     * @note 편집할 수 없는 월드와 드래그 중에는 도구·설정 명령을 차단합니다. 스케일 진입 시 로컬 좌표계로 바꾸며 그리드 명령은 아직 지원하지 않습니다.
     */
    bool ExecuteCommand(EViewportCommand Command);
    /**
     * @brief 현재 월드의 편집 가능 여부, 드래그 상태와 변환 모드에서 명령을 실행할 수 있는지 검사합니다.
     * @param[in] Command 실행 가능 여부를 조회할 동작입니다.
     * @return 처리 가능한 명령이면 true입니다. 편집 불가 월드, 드래그 중, 스케일 모드의 월드 좌표계 전환, 선택 전용 모드의 현재 모드 스냅, 미지원 명령이면 false입니다.
     */
    bool CanExecuteCommand(EViewportCommand Command) const;
    /**
     * @brief 명령에 대응하는 모드, 좌표계 또는 스냅 설정의 선택 상태를 검사합니다.
     * @param[in] Command 선택 상태를 조회할 동작입니다.
     * @return 해당 모드나 좌표계가 선택되었거나 스냅 설정이 켜져 있으면 true입니다. 좌표계 전환은 로컬일 때, 현재 모드 스냅은 활성 변환의 스냅이 켜져 있을 때 true이며 순환 및 미지원 명령은 false입니다.
     * @note 실행 가능 여부와 독립적으로 조회하므로 드래그 중에도 현재 선택을 표시할 수 있습니다.
     */
    bool IsCommandChecked(EViewportCommand Command) const;

    /**
     * @brief 클라이언트가 관리하는 변환 기즈모 모드를 반환합니다.
     * @return 현재 선택한 이동·회전·스케일 모드이며 선택 전용 모드이면 EGizmoMode::None입니다.
     */
    EGizmoMode GetGizmoMode() const;

    /**
     * @brief 클라이언트가 관리하는 기즈모 축의 좌표 공간을 반환합니다.
     * @return 기즈모 축의 좌표 공간입니다.
     * @note 스케일 모드에서는 로컬 좌표계를 유지합니다.
     */
    EGizmoSpace GetGizmoSpace() const;

    /**
     * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
     * @param[in] Input 현재 프레임의 뷰포트 입력 스냅샷입니다.
     * @param[in,out] Viewport 입력 좌표와 크기를 제공하는 뷰포트입니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    void Input(FSceneViewport* Viewport, const FViewportInput& Input, float DeltaTime) override;
    /**
     * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
     * @param[in,out] Viewport 출력 타깃과 크기를 제공하는 뷰포트입니다.
     */
    void Draw(FSceneViewport* Viewport) override;
    /**
     * @brief 기즈모 메시와 월드 메시의 CPU 광선 질의로 선택 정보를 구합니다.
     * @param[in] Viewport 입력 좌표와 크기를 제공하는 비소유 뷰포트입니다.
     * @param[in] Position 뷰포트 기준 마우스 픽셀 좌표입니다.
     * @return 기즈모 축을 우선하며 그 외에는 가장 가까운 메시 컴포넌트 또는 빈 프록시를 반환합니다.
     * @note 편집 컨텍스트에 연결된 월드를 조회하며 GPU 렌더링이나 렌더 장면 동기화를 수행하지 않습니다.
     */
    FHitProxy GetHitProxy(const FSceneViewport* Viewport, const FVector2& Position) const override;

    /**
     * @brief 현재 카메라와 뷰포트 크기로 뷰·투영 행렬을 계산합니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FSceneView CalcSceneView(const FVector2& Size) const;
    /**
     * @brief 드롭한 메시 에셋을 광선 적중점이나 배치 평면에 생성합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @param[in] Position 뷰포트 픽셀 좌표입니다.
     * @param[in] Size 쿼드의 크기 또는 뷰포트 픽셀 크기입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool DropMesh(const std::filesystem::path& Path, const FVector2& Position, const FVector2& Size);

    /**
     * @brief 에디터 카메라를 초기 위치와 회전으로 되돌립니다.
     */
    void ResetCamera();
    /**
     * @brief 유효한 위치와 회전으로 에디터 카메라를 설정합니다.
     * @param[in] Location 오류가 발생한 원래 호출 위치입니다.
     * @param[in] Rotation 적용할 오일러 또는 쿼터니언 회전입니다.
     */
    void SetCamera(const FVector& Location, const FRotator& Rotation);
    /**
     * @brief 원근 또는 정사영 뷰 방향을 설정합니다.
     * @param[in] Type 선택할 뷰 방향 또는 에셋 종류입니다.
     */
    void SetViewType(EEditorViewportType Type);
    /**
     * @brief 원근 또는 정사영 뷰 방향을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    EEditorViewportType GetViewType() const;
    /**
     * @brief 현재 뷰 방향의 표시 이름을 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    const char* GetViewTypeName() const;
    /**
     * @brief 활성 뷰포트의 기본 배치 위치를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FVector GetPlacementLocation() const;
    /**
     * @brief 에디터 카메라의 월드 위치를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FVector& GetCameraLocation() const;
    /**
     * @brief 편집 카메라의 현재 오일러 회전을 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const FRotator& GetCameraRotation() const;
    /** @brief 정사영 전환 중에도 보관한 원근 카메라를 반환합니다. @return 장면 저장에 사용할 원근 카메라입니다. */
    FEditorCameraState GetPerspectiveCamera() const;
    /** @brief 원근 카메라를 복원합니다. @param State 검증하여 복원할 카메라입니다. @return 유효한 상태를 적용하면 true입니다. */
    bool SetPerspectiveCamera(const FEditorCameraState& State);

    /**
     * @brief 뷰포트 변환 편집 기즈모를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     * @note 모드와 좌표계 변경은 ExecuteCommand()를 사용하며 반환한 기즈모는 클라이언트보다 오래 보관할 수 없습니다.
     */
    FTransformGizmo& GetGizmo();

    /**
     * @brief 이 뷰포트의 기즈모 스냅과 크기 설정을 읽기 전용으로 조회합니다.
     * @return 클라이언트가 유지하는 설정의 비소유 주소이며 클라이언트 수명 동안 유효합니다.
     * @note 설정 변경은 ExecuteCommand() 또는 SetGizmoSetting()을 사용합니다.
     */
    const FGizmoSettings* GetGizmoSettings() const;
    /**
     * @brief 기즈모 도구와 설정을 변경할 수 있는지 검사합니다.
     * @return 컨텍스트의 월드를 편집할 수 있고 진행 중인 기즈모 드래그가 없으면 true입니다.
     * @note 선택 대상의 존재 여부와 개별 명령의 모드 제한은 검사하지 않습니다.
     */
    bool CanEditGizmoSettings() const;
    /**
     * @brief 편집 가능 여부와 값의 범위를 검증하여 기즈모 수치 설정을 변경합니다.
     * @param[in] Setting 변경할 수치 항목입니다.
     * @param[in] Value 유한한 설정 값입니다.
     * @return 값을 수락하면 true이며 편집 불가 상태, 알 수 없는 항목, 유효하지 않은 값이면 설정을 유지하고 false입니다.
     * @note 스냅 간격은 양수여야 하며 크기 배율은 0.5부터 2.0까지로 제한합니다.
     */
    bool SetGizmoSetting(EGizmoSetting Setting, float Value);
    /**
     * @brief 장면의 Lit 또는 Wireframe 표시 모드를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    EViewMode GetViewMode() const;
    /**
     * @brief 장면의 Lit 또는 Wireframe 표시 모드를 설정합니다.
     * @param[in] Mode 적용할 장면 표시 모드입니다.
     */
    void SetViewMode(EViewMode Mode);
    /**
     * @brief 뷰포트별 장면·보조 표시 필터를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FEngineShowFlags& GetShowFlags() const;
    /**
     * @brief 진행 중인 기즈모 조작을 취소하고 이 뷰포트의 표시 필터를 교체합니다.
     * @param[in] Flags 적용할 뷰포트 표시 필터입니다.
     */
    void SetShowFlags(const FEngineShowFlags& Flags);
    /**
     * @brief 축과 AABB 선의 픽셀 두께를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetLineThickness() const;
    /**
     * @brief 축과 AABB 선의 픽셀 두께를 설정합니다.
     * @param[in] Thickness 픽셀 단위 선 두께입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetLineThickness(float Thickness);
    /**
     * @brief 그리드 선 사이의 월드 거리를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetGridSpacing() const;
    /**
     * @brief 그리드 선 사이의 월드 거리를 설정합니다.
     * @param[in] Spacing 그리드의 월드 단위 간격입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetGridSpacing(float Spacing);
    /**
     * @brief 마우스 이동 1픽셀당 카메라 회전 각도를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetCameraSensitivity() const;
    /**
     * @brief 마우스 이동 1픽셀당 카메라 회전 각도를 설정합니다.
     * @param[in] Sensitivity 마우스 1픽셀당 회전 각도입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetCameraSensitivity(float Sensitivity);

    /**
     * @brief 자유 시점 카메라의 기본 이동 속도를 반환합니다.
     * @return 초당 이동할 월드 단위 수이며 빠른 이동 시에는 3배를 적용합니다.
     */
    float GetCameraSpeed() const;
    /**
     * @brief 자유 시점 카메라의 기본 이동 속도를 설정합니다.
     * @param[in] Speed 0.1에서 128 사이의 유한한 초당 월드 이동 거리입니다.
     * @return 유효한 속도를 적용하면 true, 범위를 벗어나거나 유한하지 않으면 false입니다.
     */
    bool SetCameraSpeed(float Speed);

private:
    /**
     * @brief 실행 검사를 통과한 스냅 전환 명령을 기즈모 설정에 반영합니다.
     * @param[in] Command 전환할 스냅 설정의 명령입니다.
     * @return 설정을 전환하면 true이며 지원하지 않는 명령이거나 현재 모드 스냅에 대응하는 변환 모드가 없으면 false입니다.
     * @pre ExecuteCommand()에서 편집 가능 여부와 드래그 상태를 검증해야 합니다.
     */
    bool ToggleGizmoSetting(EViewportCommand Command);
    /**
     * @brief 기즈모의 이동, 회전 또는 스케일 모드를 선택합니다.
     * @param[in] Mode 적용할 변환 기즈모 모드입니다.
     * @note ExecuteCommand()의 실행 검사 후 호출하며 스케일 모드에서는 로컬 좌표계를 설정합니다.
     */
    void SetGizmoMode(EGizmoMode Mode);

    /**
     * @brief 기즈모 축의 기준을 월드 또는 로컬 좌표로 바꿉니다.
     * @param[in] Space 축에 적용할 월드 또는 로컬 좌표 공간입니다.
     * @note ExecuteCommand()에서 현재 모드와 드래그 상태를 검증한 뒤 호출합니다.
     */
    void SetGizmoSpace(EGizmoSpace Space);

    /**
     * @brief 뷰포트 입력으로 카메라 이동, 회전, 팬 또는 오빗을 적용합니다.
     * @param[in] Input 현재 프레임의 뷰포트 입력 스냅샷입니다.
     * @param[in] Size 픽셀 단위의 뷰포트 크기입니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     * @return 입력을 카메라 조작으로 소비하여 이후 선택·삭제 처리를 생략할 때 true입니다.
     * @note 자유 시점 조작은 피벗까지의 거리를 유지하며 갱신한 카메라의 시선 앞에 피벗을 배치합니다.
     */
    bool NavigateCamera(const FViewportInput& Input, const FVector2& Size, float DeltaTime);
    /**
     * @brief 선택한 액터가 화면에 들어오도록 카메라 위치와 중심을 조정합니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     */
    void FocusSelection(const FVector2& Size);
    /**
     * @brief 텍스트·Billboard의 뷰 정렬을 반영한 월드 AABB를 계산합니다.
     * @param[in] Component 경계를 계산할 컴포넌트의 비소유 주소입니다.
     * @param[in] View 카메라 행렬과 뷰포트 크기입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FBox CalcComponentBounds(const UPrimitiveComponent* Component, const FSceneView& View) const;
    /**
     * @brief 컴포넌트 경계 상자의 여덟 월드 모서리를 계산합니다.
     * @param[in] Component 경계를 계산할 컴포넌트의 비소유 주소입니다.
     * @param[in] View 카메라 행렬과 뷰포트 크기입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    std::array<FVector, 8> CalcComponentCorners(const UPrimitiveComponent* Component, const FSceneView& View) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 연결된 공통 편집 컨텍스트입니다.
     */
    FEditorContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 에디터 카메라의 월드 위치입니다.
     */
    FVector mCameraLocation{-7.0f, -9.0f, 6.0f};
    ReflectProperty(mCameraLocation, "CameraLocation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 에디터 카메라의 도 단위 회전입니다.
     */
    FRotator mCameraRotation{-26.0f, 50.0f, 0.0f};
    ReflectProperty(mCameraRotation, "CameraRotation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 오빗의 월드 중심점입니다.
     */
    FVector mOrbitPivot{};
    ReflectProperty(mOrbitPivot, "OrbitPivot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 이동 속도입니다.
     */
    float mCameraSpeed{4.0f};
    ReflectProperty(mCameraSpeed, "CameraSpeed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 마우스 이동 1픽셀당 카메라 회전 각도입니다.
     */
    float mCameraSensitivity{0.2f};
    ReflectProperty(mCameraSensitivity, "CameraSensitivity", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 장면의 Lit 또는 Wireframe 표시 모드입니다.
     */
    EViewMode mViewMode{EViewMode::Lit};
    ReflectProperty(mViewMode, "ViewMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 뷰포트별 장면·보조 표시 필터입니다.
     */
    FEngineShowFlags mShowFlags{};
    ReflectProperty(mShowFlags, "ShowFlags", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 축과 AABB 선의 픽셀 두께입니다.
     */
    float mLineThickness{1.5f};
    ReflectProperty(mLineThickness, "LineThickness", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 그리드 선 사이의 월드 거리입니다.
     */
    float mGridSpacing{1.0f};
    ReflectProperty(mGridSpacing, "GridSpacing", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 원근 또는 정사영 뷰 방향입니다.
     */
    EEditorViewportType mViewType{EEditorViewportType::Perspective};
    ReflectProperty(mViewType, "ViewType", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 정사영 전환 전 원근 카메라 위치입니다.
     */
    FVector mPerspectiveLocation{-7.0f, -9.0f, 6.0f};
    ReflectProperty(mPerspectiveLocation, "PerspectiveLocation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 정사영 전환 전 원근 카메라 회전입니다.
     */
    FRotator mPerspectiveRotation{-26.0f, 50.0f, 0.0f};
    ReflectProperty(mPerspectiveRotation, "PerspectiveRotation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 정사영 전환 전 원근 카메라 피벗입니다.
     */
    FVector mPerspectivePivot{};
    ReflectProperty(mPerspectivePivot, "PerspectivePivot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 정사영 가로 가시 범위의 월드 길이입니다.
     */
    float mOrthographicWidth{20.0f};
    ReflectProperty(mOrthographicWidth, "OrthographicWidth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 원근 카메라의 수직 시야각입니다. */
    float mFieldOfView{60.0f};
    ReflectProperty(mFieldOfView, "FieldOfView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 원근 투영의 근평면 거리입니다. */
    float mNearClip{0.05f};
    ReflectProperty(mNearClip, "NearClip", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 원근 투영의 원평면 거리입니다. */
    float mFarClip{10000.0f};
    ReflectProperty(mFarClip, "FarClip", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 클라이언트가 소유하며 기즈모와 UI가 함께 조회하는 변환 모드입니다.
     */
    EGizmoMode mGizmoMode{EGizmoMode::Translate};
    ReflectProperty(mGizmoMode, "GizmoMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 클라이언트가 소유하는 기즈모 좌표계이며 스케일 모드에서는 로컬입니다.
     */
    EGizmoSpace mGizmoSpace{EGizmoSpace::World};
    ReflectProperty(mGizmoSpace, "GizmoSpace", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 뷰포트 변환 편집 기즈모입니다.
     */
    FTransformGizmo mGizmo;
    ReflectProperty(mGizmo, "Gizmo", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 현재 프레임의 에디터 보조 선분 수집기입니다.
     **/
    FPrimitiveDrawInterface mDrawInterface{};
    ReflectProperty(mDrawInterface, "DrawInterface", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
