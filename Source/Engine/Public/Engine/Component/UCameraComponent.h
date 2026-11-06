/**
 * @file UCameraComponent.h
 * @brief 시야각 설정과 월드 변환으로부터 카메라 뷰 생성에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/USceneComponent.h"
#include "UCameraComponent.generated.h"

namespace Core {
    struct FVector2;
}

namespace Engine {

    struct FSceneView;

    /**
     * @brief 시야각 설정과 월드 변환으로부터 카메라 뷰 생성을 담당합니다.
     */
    UCLASS()
    class UCameraComponent final : public Engine::USceneComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 카메라의 수직 시야각을 도 단위로 반환합니다.
         * @return 카메라의 시야각이며 도 단위입니다.
         */
        float GetFieldOfView() const;

        /**
         * @brief 유효한 범위의 시야각을 검사하여 카메라에 설정합니다.
         * @param[in] FieldOfView 설정할 카메라 시야각이며 도 단위입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetFieldOfView(float FieldOfView);

        /**
         * @brief 카메라의 월드 변환과 시야각으로 뷰·투영 행렬을 계산합니다.
         * @param[in] Size 계산할 뷰포트의 가로·세로 픽셀 크기입니다.
         * @param[out] View 계산한 카메라 뷰·투영 상태를 받을 출력입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool GetCameraView(const Core::FVector2& Size, Engine::FSceneView& View) const;

    private:
        /**
         * @brief 카메라의 수직 시야각이며 도 단위입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetFieldOfView, Setter = SetFieldOfView)
        float mFieldOfView{60.0f};
    };

}
