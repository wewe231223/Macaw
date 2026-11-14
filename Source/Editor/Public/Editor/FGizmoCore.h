/**
 * @file FGizmoCore.h
 * @brief 편집 대상과 무관한 기즈모 핸들 표시, 교차 판정과 드래그 계산을 선언합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FQuat.h"
#include "Engine/FSceneView.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FHitProxy.h"
#include "Editor/FGizmoSettings.h"

#include <array>

namespace Engine {
    class AActor;
}

namespace Editor {

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
        /**
         * @brief 기즈모를 표시하지 않는 선택 전용 모드입니다.
         */
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
     * @brief 기즈모를 놓을 위치, 축과 모드를 편집 대상과 무관하게 나타냅니다.
     * @ingroup Editor
     */
    struct FGizmoPlacement {
    public:
        /**
         * @brief 기즈모의 월드 중심점입니다.
         */
        Core::FVector mPivot{};

        /**
         * @brief 기즈모의 세 단위 축입니다.
         */
        std::array<Core::FVector, 3> mAxes{};

        /**
         * @brief 표시하고 드래그할 변환 모드입니다.
         */
        Editor::EGizmoMode mMode{Editor::EGizmoMode::Translate};

        /**
         * @brief 핸들의 HitProxy에 기록할 대상 객체의 핸들이며 객체가 아닌 대상은 비워 둡니다.
         */
        CoreUObject::TObjectRef<Engine::AActor> mOwner{};
    };

    /**
     * @brief 핸들 표시와 교차 판정, 마우스 드래그를 이동량·회전량·스케일 변화량으로 바꾸는 계산을 제공합니다.
     * @ingroup Editor
     * @note 계산 결과를 편집 대상에 적용하는 일은 호출자가 맡습니다.
     */
    class FGizmoCore {
    public:
        /**
         * @brief 기즈모의 기본 X·Y·Z 단위 축을 반환합니다.
         * @return 월드 좌표계의 세 단위 축입니다.
         */
        static const std::array<Core::FVector, 3>& GetUnitAxes();

        /**
         * @brief 기즈모 핸들이 주어진 로컬 축에 영향을 주는지 검사합니다.
         * @param[in] Handle 기즈모 핸들 번호입니다.
         * @param[in] Axis 검사할 축의 인덱스입니다.
         * @return 조건을 만족하면 true입니다.
         */
        static bool Affects(Core::Int32 Handle, Core::Index_t Axis);

        /**
         * @brief 핸들의 CPU 레이 교차 판정으로 마우스가 가리키는 핸들을 구합니다.
         * @param[in] Mouse 뷰포트 기준 마우스 픽셀 좌표입니다.
         * @param[in] View 입력 해석에 사용할 카메라 뷰입니다.
         * @param[in] Placement 기즈모의 위치, 축과 모드입니다.
         * @return 적중한 핸들을 나타내는 프록시이며 없으면 빈 프록시입니다.
         */
        Engine::FHitProxy HitTest(const Core::FVector2& Mouse, const Engine::FSceneView& View, const Editor::FGizmoPlacement& Placement) const;

        /**
         * @brief 선택 축의 시작 상태를 저장하고 드래그 평면을 구성합니다.
         * @param[in] Axis 드래그를 시작할 기즈모 축 번호입니다.
         * @param[in] Mouse 뷰포트 기준 마우스 시작 위치입니다.
         * @param[in] View 입력 해석에 사용할 카메라 뷰입니다.
         * @param[in] Placement 드래그 시작 시점의 위치, 축과 모드입니다.
         * @return 드래그 상태를 준비하면 true입니다.
         */
        bool BeginDrag(Core::Index_t Axis, const Core::FVector2& Mouse, const Engine::FSceneView& View, const Editor::FGizmoPlacement& Placement);

        /**
         * @brief 마우스 위치를 활성 모드의 이동량, 회전량 또는 스케일 변화량으로 바꿉니다.
         * @param[in] Mouse 뷰포트 기준 현재 마우스 위치입니다.
         * @return 새 결과를 계산했으면 true이며 false이면 이전 결과를 유지합니다.
         */
        bool UpdateDrag(const Core::FVector2& Mouse);

        /**
         * @brief 드래그와 호버 상태를 비웁니다.
         */
        void EndDrag();

        /**
         * @brief 카메라와 모드에 맞는 핸들 도형과 히트 프록시를 추가합니다.
         * @param[in] View 핸들 크기와 방향을 계산할 카메라 뷰입니다.
         * @param[in] Placement 기즈모의 위치, 축과 모드이며 드래그 중에는 시작 시점의 축을 대신 사용합니다.
         * @param[in,out] DrawInterface 도형을 받을 그리기 인터페이스입니다.
         */
        void BuildHandles(const Engine::FSceneView& View, const Editor::FGizmoPlacement& Placement, Engine::FPrimitiveDrawInterface& DrawInterface) const;

        /**
         * @brief 진행 중인 드래그의 이동·회전 피드백을 표시합니다.
         * @param[in,out] DrawInterface 선을 받을 그리기 인터페이스입니다.
         */
        void DrawFeedback(Engine::FPrimitiveDrawInterface& DrawInterface) const;

        /**
         * @brief 활성 축을 드래그하고 있는지 검사합니다.
         * @return 드래그 중이면 true입니다.
         */
        bool IsDragging() const;

        /**
         * @brief 드래그 중인 핸들 인덱스를 반환합니다.
         * @return 드래그 중인 핸들 인덱스, 없으면 -1입니다.
         */
        Core::Index_t GetActiveAxis() const;

        /**
         * @brief 마우스가 가리키는 핸들 인덱스를 반환합니다.
         * @return 마우스가 가리키는 핸들 인덱스, 없으면 -1입니다.
         */
        Core::Index_t GetHoveredAxis() const;

        /**
         * @brief 강조할 호버 핸들을 지정합니다.
         * @param[in] Axis 마우스가 가리키는 핸들 인덱스, 없으면 -1입니다.
         */
        void SetHoveredAxis(Core::Index_t Axis);

        /**
         * @brief 드래그 시작 시 고정한 변환 모드를 반환합니다.
         * @return 드래그 시작 시의 변환 모드입니다.
         */
        Editor::EGizmoMode GetDragMode() const;

        /**
         * @brief 드래그 시작 마우스 좌표를 반환합니다.
         * @return 뷰포트 기준 시작 픽셀 좌표입니다.
         */
        const Core::FVector2& GetStartMouse() const;

        /**
         * @brief 드래그 시작 시 고정한 피벗을 반환합니다.
         * @return 시작 월드 피벗입니다.
         */
        const Core::FVector& GetStartPivot() const;

        /**
         * @brief 드래그 시작 시 고정한 세 기저 축을 반환합니다.
         * @return 시작 시점의 기저 축입니다.
         */
        const std::array<Core::FVector, 3>& GetDragAxes() const;

        /**
         * @brief 현재 드래그의 누적 월드 이동량을 반환합니다.
         * @return 시작 피벗 기준 이동량입니다.
         */
        const Core::FVector& GetTranslation() const;

        /**
         * @brief 현재 드래그의 스냅을 적용한 월드 회전량을 반환합니다.
         * @return 시작 시점 기준 회전입니다.
         */
        Core::FQuat GetRotation() const;

        /**
         * @brief 현재 드래그의 스냅을 적용한 스케일 변화량을 반환합니다.
         * @return 시작 시점 기준 스케일 증가량입니다.
         */
        float GetScaleDelta() const;

        /**
         * @brief 스냅과 크기 설정을 반환합니다.
         * @return 이 객체가 유지하는 설정의 비소유 주소입니다.
         */
        const Editor::FGizmoSettings* GetSettings() const;

        /**
         * @brief 호출자가 검증한 설정의 사본을 저장합니다.
         * @param[in] Settings 적용할 설정입니다.
         */
        void SetSettings(const Editor::FGizmoSettings& Settings);

    private:
        /**
         * @brief 피벗 깊이와 시야각으로 화면 픽셀당 월드 거리를 구합니다.
         * @param[in] View 입력 해석이나 그리기에 사용할 카메라 뷰입니다.
         * @param[in] Pivot 기즈모의 월드 중심점입니다.
         * @return 기즈모 크기 배율을 반영한 픽셀당 월드 거리이며 카메라 뒤에서는 0입니다.
         */
        float GetSize(const Engine::FSceneView& View, const Core::FVector& Pivot) const;

    private:
        /**
         * @brief 마우스가 가리키는 핸들 인덱스, 없으면 -1입니다.
         */
        Core::Index_t mHoveredAxis{-1};

        /**
         * @brief 드래그 중인 핸들 인덱스, 없으면 -1입니다.
         */
        Core::Index_t mActiveAxis{-1};

        /**
         * @brief 드래그 시작 시 카메라 뷰입니다.
         */
        Engine::FSceneView mDragView{};

        /**
         * @brief 드래그 시작 마우스 픽셀 좌표입니다.
         */
        Core::FVector2 mStartMouse{};

        /**
         * @brief 드래그할 월드 단위 축입니다.
         */
        Core::FVector mAxis{};

        /**
         * @brief 드래그 평면의 단위 법선입니다.
         */
        Core::FVector mPlaneNormal{};

        /**
         * @brief 드래그 시작 광선과 평면의 교점입니다.
         */
        Core::FVector mStartIntersection{};

        /**
         * @brief 드래그 기준 기즈모 크기입니다.
         */
        float mDragSize{1.0f};

        /**
         * @brief 이동·회전·스케일 스냅 간격과 기즈모 크기 설정입니다.
         */
        Editor::FGizmoSettings mSettings{};

        /**
         * @brief 드래그를 시작할 때 고정한 이동, 회전과 스케일의 기준 피벗입니다.
         */
        Core::FVector mStartPivot{};

        /**
         * @brief 회전 드래그의 시작 반지름 방향입니다.
         */
        Core::FVector mStartDirection{};

        /**
         * @brief 회전 누적각을 계산할 때 사용할 이전 프레임의 반지름 방향입니다.
         */
        Core::FVector mPreviousDirection{};

        /**
         * @brief 현재 드래그에서 시작 피벗에 더할 누적 월드 이동량입니다.
         */
        Core::FVector mTranslation{};

        /**
         * @brief 드래그 시작 시 고정하여 대상 회전과 독립적으로 유지할 세 기저 축입니다.
         */
        std::array<Core::FVector, 3> mDragAxes{};

        /**
         * @brief 퇴화한 드래그 평면의 대체 계산이나 스케일에 사용할 화면 방향입니다.
         */
        Core::FVector2 mScreenAxis{};

        /**
         * @brief 각 프레임의 회전을 더하여 여러 바퀴 회전도 유지하는 라디안 각도입니다.
         */
        float mRawAngle{};

        /**
         * @brief 각도 스냅을 적용한 실제 라디안 회전량입니다.
         */
        float mAppliedAngle{};

        /**
         * @brief 스냅을 적용한 스케일 변화량입니다.
         */
        float mScaleDelta{};

        /**
         * @brief 드래그 시작 시 고정한 변환 모드입니다.
         */
        Editor::EGizmoMode mDragMode{Editor::EGizmoMode::Translate};

        /**
         * @brief 광선과 조작 평면이 거의 평행하여 화면 방향 계산을 사용할지 여부입니다.
         */
        bool mScreenFallback{};
    };

}
