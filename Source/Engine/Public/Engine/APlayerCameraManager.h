/**
 * @file APlayerCameraManager.h
 * @brief 뷰 타깃에서 매 프레임 카메라 시점을 계산하고 캐시하는 카메라 관리자입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FVector.h"
#include "Engine/AActor.h"
#include "CoreUObject/FObjectHandle.h"
#include "Engine/EViewTargetBlendFunction.h"
#include "Engine/FMinimalViewInfo.h"
#include "Engine/UCameraModifier.h"
#include <memory>
#include "APlayerCameraManager.generated.h"

namespace Engine {

    class UCameraShakeModifier;

    /**
     * @brief 뷰 타깃에서 매 프레임 카메라 시점을 계산하고 캐시하는 카메라 관리자입니다.
     * @details Game·PIE 월드가 실행을 시작할 때 영속 레벨에 만들고 종료할 때 파괴합니다.
     *          뷰 타깃 액터의 첫 UCameraComponent를 시점으로 쓰고, 카메라가 없으면 액터 변환을 씁니다.
     *          월드가 액터 Tick을 모두 마친 뒤 갱신하므로 이번 프레임의 이동이 반영됩니다. 게임 뷰포트는
     *          캐시가 있으면 이 시점으로 그리고, 없으면 월드의 활성 카메라로 그립니다.
     *
     *          시점을 확정하는 순서는 뷰 타깃 시점 계산, 뷰 타깃 블렌드, 카메라 모디파이어 적용, 캐시 저장입니다.
     *          기본으로 UCameraShakeModifier가 하나 추가되어 있습니다.
     * @ingroup Engine
     */
    UCLASS(NoFactory)
    class APlayerCameraManager final : public Engine::AActor {
        GENERATED_BODY()

    public:
        /**
         * @brief 뷰 타깃이 없는 카메라 관리자를 구성합니다.
         */
        APlayerCameraManager();

        /**
         * @brief 객체의 수명을 끝냅니다.
         */
        ~APlayerCameraManager() override;

    public:
        /**
         * @brief 현재 뷰 타깃을 반환합니다. 블렌드 중이면 전환할 대상을 반환합니다.
         * @return 뷰 타깃의 비소유 주소이며, 없으면 nullptr입니다.
         */
        UFUNCTION()
        Engine::AActor* GetViewTarget() const;

        /**
         * @brief 블렌드 없이 뷰 타깃을 바꿉니다.
         * @param[in] Target 새 뷰 타깃이며 nullptr이면 뷰 타깃을 해제합니다.
         * @return 같은 월드의 액터이거나 nullptr이어서 바꿨으면 true입니다.
         */
        UFUNCTION()
        bool SetViewTarget(Engine::AActor* Target);

        /**
         * @brief 현재 시점에서 새 뷰 타깃으로 지정한 시간 동안 블렌드합니다.
         * @param[in] Target 새 뷰 타깃입니다.
         * @param[in] BlendTime 블렌드 시간이며 초 단위입니다. 0 이하이면 바로 바꿉니다.
         * @param[in] Function 블렌드 곡선입니다.
         * @return 블렌드를 시작했거나 바로 바꿨으면 true입니다.
         */
        UFUNCTION()
        bool SetViewTargetWithBlend(Engine::AActor* Target, float BlendTime, Engine::EViewTargetBlendFunction Function);

        /**
         * @brief 뷰 타깃 블렌드가 진행 중인지 검사합니다.
         * @return 블렌드 중이면 true입니다.
         */
        UFUNCTION()
        bool IsBlending() const;

        /**
         * @brief 마지막으로 계산한 카메라 위치를 반환합니다.
         * @return 월드 위치이며 캐시가 없으면 원점입니다.
         */
        UFUNCTION()
        Core::FVector GetCameraLocation() const;

        /**
         * @brief 마지막으로 계산한 카메라 회전을 반환합니다.
         * @return 월드 회전이며 캐시가 없으면 단위 회전입니다.
         */
        UFUNCTION()
        Core::FQuat GetCameraRotation() const;

        /**
         * @brief 마지막으로 계산한 수직 시야각을 반환합니다.
         * @return 도 단위 시야각입니다.
         */
        UFUNCTION()
        float GetFieldOfView() const;

        /**
         * @brief 뷰 타깃에서 시점을 다시 계산해 캐시합니다.
         * @param[in] DeltaTime 블렌드를 진행할 초 단위 시간입니다.
         */
        void UpdateCamera(float DeltaTime);

        /**
         * @brief 마지막 갱신에서 유효한 시점을 계산했는지 검사합니다.
         * @return 캐시가 있으면 true입니다.
         */
        bool HasCameraCache() const;

        /**
         * @brief 마지막으로 계산한 시점을 반환합니다.
         * @return 캐시한 시점의 읽기 전용 참조입니다.
         */
        const Engine::FMinimalViewInfo& GetCameraCacheView() const;

        /**
         * @brief 기본 카메라 셰이크 모디파이어를 만들어 추가합니다.
         * @return 추가했으면 true입니다.
         */
        bool InitializeDefaultComponents() override;

        /**
         * @brief 이미 만든 모디파이어의 소유권을 넘겨받아 추가합니다.
         * @param[in] Modifier 추가할 모디파이어이며 다른 카메라 관리자에 추가된 적이 없어야 합니다.
         * @return 추가한 모디파이어의 비소유 주소이며, AddedToCamera가 거부하면 nullptr입니다.
         */
        Engine::UCameraModifier* AddCameraModifier(std::unique_ptr<Engine::UCameraModifier> Modifier);

        /**
         * @brief 지정한 클래스의 모디파이어를 만들어 추가합니다.
         * @param[in] Class UCameraModifier 파생이고 팩터리가 있는 클래스입니다.
         * @return 추가한 모디파이어의 비소유 주소이며, 만들 수 없거나 거부되면 nullptr입니다.
         */
        Engine::UCameraModifier* AddNewCameraModifier(const CoreUObject::UClass* Class);

        /**
         * @brief 모디파이어를 목록에서 빼고 해제합니다.
         * @param[in] Modifier 뺄 모디파이어입니다.
         * @return 목록에 있어서 뺐으면 true입니다.
         * @note ModifyCamera 안에서 불러도 되며, 그 경우 이번 갱신이 끝난 뒤에 해제합니다.
         */
        UFUNCTION()
        bool RemoveCameraModifier(Engine::UCameraModifier* Modifier);

        /**
         * @brief 지정한 클래스의 첫 모디파이어를 찾습니다.
         * @param[in] Class 찾을 클래스이며 파생 클래스도 일치합니다.
         * @return 모디파이어의 비소유 주소이며, 없으면 nullptr입니다.
         */
        Engine::UCameraModifier* FindCameraModifierByClass(const CoreUObject::UClass* Class) const;

        /**
         * @brief 기본으로 추가되는 카메라 셰이크 모디파이어를 반환합니다.
         * @return 모디파이어의 비소유 주소이며, 제거했으면 nullptr입니다.
         */
        UFUNCTION()
        Engine::UCameraShakeModifier* GetCameraShakeModifier() const;

    private:
        /**
         * @brief 모디파이어를 적용하기 전의 시점에 모디파이어를 적용해 캐시를 확정합니다.
         * @param[in] DeltaTime 이번 갱신의 초 단위 시간입니다.
         * @param[in] HasBaseView mBaseView를 계산했으면 true입니다.
         */
        void FinishUpdate(float DeltaTime, bool HasBaseView);

        /**
         * @brief 켜진 모디파이어를 Priority 순서로 시점에 적용합니다.
         * @param[in] DeltaTime 이번 갱신의 초 단위 시간입니다.
         * @param[in,out] ViewInfo 고칠 시점입니다.
         */
        void ApplyCameraModifiers(float DeltaTime, Engine::FMinimalViewInfo& ViewInfo);

        /**
         * @brief 핸들을 이 월드에 살아 있는 액터로 해석합니다.
         * @param[in] Handle 해석할 액터 핸들입니다.
         * @return 살아 있는 액터이며, 없으면 nullptr입니다.
         */
        Engine::AActor* ResolveTarget(CoreUObject::FObjectHandle Handle) const;

        /**
         * @brief 액터의 카메라나 변환으로 시점을 계산합니다.
         * @param[in] Target 시점을 계산할 액터입니다.
         * @param[out] ViewInfo 계산한 시점입니다.
         * @return 계산했으면 true입니다.
         */
        bool CalcViewTargetPOV(const Engine::AActor* Target, Engine::FMinimalViewInfo& ViewInfo) const;

        /**
         * @brief 진행 중인 블렌드를 끝내고 전환 대상을 뷰 타깃으로 확정합니다.
         */
        void FinishBlend();

    private:
        /**
         * @brief 현재 뷰 타깃이며, 블렌드 중에는 출발 대상입니다.
         */
        CoreUObject::FObjectHandle mViewTarget{};

        /**
         * @brief 블렌드로 전환할 뷰 타깃입니다.
         */
        CoreUObject::FObjectHandle mPendingViewTarget{};

        /**
         * @brief 블렌드 출발 시점이며, 출발 대상이 사라지면 이 값을 씁니다.
         */
        Engine::FMinimalViewInfo mBlendFrom{};

        /**
         * @brief 블렌드 전체 시간이며 초 단위입니다.
         */
        float mBlendTime{};

        /**
         * @brief 블렌드를 시작한 뒤 지난 시간이며 초 단위입니다.
         */
        float mBlendElapsed{};

        /**
         * @brief 블렌드 곡선입니다.
         */
        Engine::EViewTargetBlendFunction mBlendFunction{Engine::EViewTargetBlendFunction::Linear};

        /**
         * @brief 블렌드가 진행 중인지 나타냅니다.
         */
        bool mBlending{};

        /**
        /**
         * @brief 모디파이어를 적용하기 전의 시점이며, 새 블렌드의 출발점으로 씁니다.
         */
        Engine::FMinimalViewInfo mBaseView{};

        /**
         * @brief 마지막으로 계산한 시점입니다.
         */
        Engine::FMinimalViewInfo mCache{};

        /**
         * @brief mCache가 유효한지 나타냅니다.
         */
        bool mHasCache{};

        /**
         * @brief 뷰 타깃에 카메라가 없을 때 쓰는 수직 시야각이며 도 단위입니다.
         */
        UPROPERTY(Edit)
        float mDefaultFieldOfView{60.0f};

        /**
         * @brief 적용 순서대로 정렬하는 모디파이어 목록입니다.
         */
        Core::TArray<std::unique_ptr<Engine::UCameraModifier>> mModifiers{};

        /**
         * @brief 적용 중에 제거되어 갱신이 끝난 뒤 해제할 모디파이어입니다.
         */
        Core::TArray<std::unique_ptr<Engine::UCameraModifier>> mRemovedModifiers{};

        /**
         * @brief 기본 카메라 셰이크 모디파이어입니다.
         */
        Engine::UCameraShakeModifier* mCameraShake{};
    };

}
