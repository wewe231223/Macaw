/**
 * @file ViewportClient.h
 * @brief 에디터 카메라, 선택, 공통 명령과 기즈모 입력을 처리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FViewportClient.h"
#include "Editor/Gizmo.h"
#include "Editor/EViewportCommand.h"

class UEditorEngine;

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
     * @param[in,out] Engine 패널 또는 뷰포트가 참조할 에디터 엔진입니다.
     */
    explicit FEditorViewportClient(UEditorEngine* Engine);
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

    /**
     * @brief 실행 가능 여부를 검사한 뒤 뷰포트 명령을 처리합니다.
     * @param[in] Command 툴바, 메뉴 또는 단축키에서 요청한 동작입니다.
     * @return 명령을 처리하면 true이며 차단되거나 지원하지 않는 명령이면 상태를 변경하지 않고 false입니다.
     * @note 드래그 중에는 변환 명령을 차단합니다. 스케일 진입 시 로컬 좌표계로 바꾸며 그리드 명령은 아직 지원하지 않습니다.
     */
    bool ExecuteCommand(EViewportCommand Command);
    /**
     * @brief 현재 드래그 상태와 변환 모드에서 명령을 실행할 수 있는지 검사합니다.
     * @param[in] Command 실행 가능 여부를 조회할 동작입니다.
     * @return 처리 가능한 명령이면 true이며 드래그 중이거나 스케일 모드의 월드 좌표계 전환, 미지원 명령이면 false입니다.
     */
    bool CanExecuteCommand(EViewportCommand Command) const;
    /**
     * @brief 명령에 대응하는 모드 또는 좌표계가 선택되어 있는지 검사합니다.
     * @param[in] Command 선택 상태를 조회할 동작입니다.
     * @return 해당 모드나 좌표계가 선택되어 있으면 true입니다. 좌표계 전환 명령은 로컬일 때 true이며 순환 및 미지원 명령은 false입니다.
     * @note 실행 가능 여부와 독립적으로 조회하므로 드래그 중에도 현재 선택을 표시할 수 있습니다.
     */
    bool IsCommandChecked(EViewportCommand Command) const;

    /**
     * @brief 클라이언트가 관리하는 변환 기즈모 모드를 반환합니다.
     * @return 이동, 회전 또는 스케일 중 현재 선택한 모드입니다.
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
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    void Input(const FViewportInput& Input, const FVector2& Size, float DeltaTime) override;
    /**
     * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
     * @param[in,out] Viewport 출력 타깃과 크기를 제공하는 뷰포트입니다.
     */
    void Draw(FSceneViewport* Viewport) override;

    /**
     * @brief 현재 카메라와 뷰포트 크기로 뷰·투영 행렬을 계산합니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FSceneView CalcSceneView(const FVector2& Size) const;

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
     * @brief 에디터 카메라의 월드 위치를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FVector& GetCameraLocation() const;

    /**
     * @brief 뷰포트 변환 편집 기즈모를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     * @note 모드와 좌표계 변경은 ExecuteCommand()를 사용하며 반환한 기즈모는 클라이언트보다 오래 보관할 수 없습니다.
     */
    FTransformGizmo& GetGizmo();

private:
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
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool NavigateCamera(const FViewportInput& Input, const FVector2& Size, float DeltaTime);
    /**
     * @brief 선택한 액터가 화면에 들어오도록 카메라 위치와 중심을 조정합니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     */
    void FocusSelection(const FVector2& Size);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 연결된 에디터 엔진입니다.
     */
    UEditorEngine* mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

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
     */
    FPrimitiveDrawInterface mDrawInterface{};
    ReflectProperty(mDrawInterface, "DrawInterface", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
