/**
 * @file Gizmo.h
 * @brief 이동, 회전과 스케일 기즈모의 화면 질의와 드래그를 구현합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectHandle.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FTransform.h"
#include "Engine/FSceneView.h"
#include "Engine/FWorldHandle.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FViewportInput.h"
#include "Engine/FHitProxy.h"
#include "Editor/FGizmoSettings.h"

class FEditorContext;
class USceneComponent;
class FEditorViewportClient;

/**
 * @brief 기즈모로 편집할 이동, 회전 또는 스케일 연산을 지정합니다.
 * @ingroup Editor
 */
enum class EGizmoMode {
    /**
     * @brief 위치를 편집합니다.
     */
    Translate,
    /**
     * @brief 회전을 편집합니다.
     */
    Rotate,
    /**
     * @brief 축별 스케일을 편집합니다.
     */
    Scale,
    None
};

/**
 * @brief 기즈모 축을 월드 좌표 또는 객체 로컬 좌표로 지정합니다.
 * @ingroup Editor
 */
enum class EGizmoSpace {
    /**
     * @brief 월드 좌표축을 사용합니다.
     */
    World,
    /**
     * @brief 대상의 회전이 적용된 로컬 좌표축을 사용합니다.
     */
    Local
};

/**
 * @brief 이동, 회전과 스케일 축의 선택 및 드래그 편집을 제공합니다.
 * @ingroup Editor
 * @note 변환 모드와 좌표계는 FEditorViewportClient가 소유하며 기즈모는 해당 상태를 읽어 사용합니다.
 */
class FTransformGizmo {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FTransformGizmo);

private:
    struct FTargetState {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(FTargetState);

    public:
        ReflectStruct(FTargetState);

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();

    public:
        FObjectHandle mObject{};
        ReflectProperty(mObject, "Object", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        FTransform mTransform{};
        ReflectProperty(mTransform, "Transform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    /**
     * @brief FTransformGizmo의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in,out] Context 패널 또는 뷰포트가 공유할 편집 컨텍스트입니다.
     * @param[in] Client 변환 모드와 좌표계를 제공하는 뷰포트 클라이언트의 비소유 주소입니다.
     * @pre Context와 Client는 nullptr이 아니며 기즈모를 사용하는 동안 유효해야 합니다.
     */
    FTransformGizmo(FEditorContext* Context, const FEditorViewportClient* Client);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FTransformGizmo);
    void SetEditorContext(FEditorContext* Context);

    /**
     * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
     * @param[in] Input 현재 프레임의 뷰포트 입력 스냅샷입니다.
     * @param[in] View 입력 해석이나 그리기에 사용할 카메라 뷰입니다.
     * @param[in] Hit CPU 교차 판정으로 찾은 객체 또는 기즈모 축입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Input(const FViewportInput& Input, const FSceneView& View, const FHitProxy& Hit);
    /**
     * @brief 그리는 메시의 CPU 레이 교차 판정으로 선택 액터의 기즈모 HitProxy를 구합니다.
     * @param[in] Mouse 뷰포트 기준 마우스 픽셀 좌표입니다.
     * @param[in] View 입력 해석이나 그리기에 사용할 카메라 뷰입니다.
     * @return 선택 액터와 기즈모 축을 나타내며 편집할 수 없거나 유효한 루트 또는 적중 축이 없으면 빈 프록시입니다.
     */
    FHitProxy HitTest(const FVector2& Mouse, const FSceneView& View) const;
    /**
     * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
     * @param[in] View 입력 해석이나 그리기에 사용할 카메라 뷰입니다.
     * @param[in,out] DrawInterface 보조 선분을 추가할 그리기 인터페이스입니다.
     */
    void Draw(const FSceneView& View, FPrimitiveDrawInterface& DrawInterface);

    /**
     * @brief 진행 중인 기즈모 드래그를 취소하고 가능한 경우 시작 변환을 복원합니다.
     */
    void Cancel();
    /**
     * @brief 기즈모에서 활성 축을 드래그하고 있는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsDragging() const;
    /**
     * @brief 이 기즈모의 스냅과 크기 설정을 읽기 전용으로 반환합니다.
     * @return 기즈모가 유지하는 설정의 비소유 주소이며 기즈모 수명 동안 유효합니다.
     * @note 설정 변경은 소유 뷰포트 클라이언트의 명령 또는 수치 설정 API를 사용합니다.
     */
    const FGizmoSettings* GetSettings() const;
    FVector GetPivot() const;
    bool IsPivotDrag() const;
    /**
     * @brief 마우스가 가리키는 핸들 인덱스, 없으면 -1을 반환합니다.
     * @return 마우스가 가리키는 핸들 인덱스, 없으면 -1입니다.
     */
    Int32 GetHoveredAxis() const;

private:
    /**
     * @brief 편집 권한과 값의 범위를 검증한 뷰포트 클라이언트에 설정 저장을 허용합니다.
     */
    friend class FEditorViewportClient;

    /**
     * @brief 기존 대상 스냅샷을 비우고 현재 선택된 변환 대상들의 핸들과 변환을 수집합니다.
     * @return 유효한 장면 컴포넌트를 하나 이상 수집하면 true입니다.
     * @note 드래그 시작과 Alt 복제 직후에 사용하며 해석할 수 없는 대상은 제외합니다.
     */
    bool CaptureTargets();
    /**
     * @brief 뷰포트 클라이언트가 검증한 설정의 사본을 저장합니다.
     * @param[in] Settings 이 기즈모에 적용할 설정입니다.
     * @pre 호출자가 편집 가능 여부와 각 설정 값의 유효 범위를 검증해야 합니다.
     */
    void SetSettings(const FGizmoSettings& Settings);
    /**
     * @brief 선택한 좌표 공간에 맞는 기즈모 단위 축을 구합니다.
     * @param[in] Transform 적용할 위치, 회전과 스케일입니다.
     * @param[in] Index 선택할 요소 또는 축의 0부터 시작하는 인덱스입니다.
     * @return 선택한 좌표 공간에 맞는 기즈모 단위 축을 구합니다.입니다.
     */
    FVector GetAxis(const FTransform& Transform, Int32 Index) const;
    /**
     * @brief 피벗 깊이와 시야각으로 화면 픽셀당 월드 거리를 구합니다.
     * @param[in] View 입력 해석이나 그리기에 사용할 카메라 뷰입니다.
     * @param[in] Pivot 기즈모 또는 카메라 회전의 월드 중심점입니다.
     * @return 기즈모 크기 배율을 반영한 픽셀당 월드 거리이며 카메라 뒤에서는 0입니다.
     */
    float GetSize(const FSceneView& View, const FVector& Pivot) const;

    /**
     * @brief 선택 축과 대상의 시작 변환을 저장하고 드래그 평면을 구성합니다.
     * @param[in] Axis 회전 또는 편집에 사용할 축입니다.
     * @param[in] Input 마우스 시작 위치, 누른 버튼과 복제 키 상태를 제공합니다.
     * @param[in] View 입력 해석이나 그리기에 사용할 카메라 뷰입니다.
     * @return 드래그 대상과 시작 상태를 준비하면 true입니다.
     */
    bool BeginDrag(Int32 Axis, const FViewportInput* Input, const FSceneView& View);
    /**
     * @brief 마우스 이동량을 활성 모드의 변환으로 바꾸어 선택한 액터에 적용합니다.
     * @param[in] Input 현재 뷰포트 기준 마우스 위치를 제공합니다.
     */
    void UpdateDrag(const FViewportInput* Input);
    void EndDrag(bool Restore);
    USceneComponent* GetPrimaryTarget() const;
    void BuildHandles(const FSceneView& View, FPrimitiveDrawInterface* DrawInterface) const;
    void DrawFeedback(FPrimitiveDrawInterface* DrawInterface) const;

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
     * @brief 변환 모드와 좌표계를 읽을 뷰포트 클라이언트의 비소유 주소입니다.
     */
    const FEditorViewportClient* mClient{};
    ReflectProperty(mClient, "ViewportClient", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 마우스가 가리키는 핸들 인덱스, 없으면 -1입니다.
     */
    Int32 mHoveredAxis{-1};
    ReflectProperty(mHoveredAxis, "HoveredAxis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 중인 핸들 인덱스, 없으면 -1입니다.
     */
    Int32 mActiveAxis{-1};
    ReflectProperty(mActiveAxis, "ActiveAxis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 시작 시 편집 대상 월드의 세대 핸들입니다.
     */
    FWorldHandle mWorldHandle{};
    ReflectProperty(mWorldHandle, "WorldHandle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 시작 시 대상 변환입니다.
     */
    FTransform mStartTransform{};
    ReflectProperty(mStartTransform, "StartTransform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 시작 시 카메라 뷰입니다.
     */
    FSceneView mDragView{};
    ReflectProperty(mDragView, "DragView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 시작 마우스 픽셀 좌표입니다.
     */
    FVector2 mStartMouse{};
    ReflectProperty(mStartMouse, "StartMouse", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그할 월드 단위 축입니다.
     */
    FVector mAxis{};
    ReflectProperty(mAxis, "Axis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 평면의 단위 법선입니다.
     */
    FVector mPlaneNormal{};
    ReflectProperty(mPlaneNormal, "PlaneNormal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 시작 광선과 평면의 교점입니다.
     */
    FVector mStartIntersection{};
    ReflectProperty(mStartIntersection, "StartIntersection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 기준 기즈모 크기입니다.
     */
    float mDragSize{1.0f};
    ReflectProperty(mDragSize, "DragSize", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FGizmoSettings mSettings{};
    ReflectProperty(mSettings, "Settings", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 드래그 시작 시 기록한 편집 대상과 원본 변환입니다.
     */
    TArray<FTargetState> mTargets{};
    ReflectProperty(mTargets, "Targets", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FObjectHandle> mOriginalSelection{};
    ReflectProperty(mOriginalSelection, "OriginalSelection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FObjectHandle> mDuplicates{};
    ReflectProperty(mDuplicates, "Duplicates", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FObjectHandle mOriginalObject{};
    ReflectProperty(mOriginalObject, "OriginalObject", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mSelectionRevision{};
    ReflectProperty(mSelectionRevision, "SelectionRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mPivotRevision{};
    ReflectProperty(mPivotRevision, "PivotRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mPivot{};
    ReflectProperty(mPivot, "Pivot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mStartPivot{};
    ReflectProperty(mStartPivot, "StartPivot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mOriginalPivot{};
    ReflectProperty(mOriginalPivot, "OriginalPivot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mStartDirection{};
    ReflectProperty(mStartDirection, "StartDirection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mPreviousDirection{};
    ReflectProperty(mPreviousDirection, "PreviousDirection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mTranslation{};
    ReflectProperty(mTranslation, "Translation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<FVector, 3> mDragAxes{};
    ReflectProperty(mDragAxes, "DragAxes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector2 mScreenAxis{};
    ReflectProperty(mScreenAxis, "ScreenAxis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mRawAngle{};
    ReflectProperty(mRawAngle, "RawAngle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mAppliedAngle{};
    ReflectProperty(mAppliedAngle, "AppliedAngle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    EGizmoMode mDragMode{EGizmoMode::Translate};
    ReflectProperty(mDragMode, "DragMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mHasPivot{};
    ReflectProperty(mHasPivot, "HasPivot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mHadPivot{};
    ReflectProperty(mHadPivot, "HadPivot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mPivotOnly{};
    ReflectProperty(mPivotOnly, "PivotOnly", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mDuplicatePending{};
    ReflectProperty(mDuplicatePending, "DuplicatePending", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mScreenFallback{};
    ReflectProperty(mScreenFallback, "ScreenFallback", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
