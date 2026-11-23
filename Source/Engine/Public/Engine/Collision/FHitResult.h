/**
 * @file FHitResult.h
 * @brief 충돌 이벤트의 접촉 정보와 비소유 객체 참조입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FVector.h"
#include "CoreUObject/TObjectRef.h"
#include "Engine/Component/UPrimitiveComponent.h"

namespace Engine {

    class AActor;

    /**
     * @brief 충돌 이벤트의 접촉 정보와 비소유 객체 참조입니다.
     */
    struct FHitResult {
    public:
        /**
         * @brief 전달된 설정과 기본값으로 FHitResult 객체를 생성합니다.
         */
        FHitResult() = default;

    public:
        /**
         * @brief 적중한 액터의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::AActor* GetActor() const;

        /**
         * @brief 적중한 프리미티브 컴포넌트의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::UPrimitiveComponent* GetComponent() const;

        /**
         * @brief 충돌 결과의 액터를 세대 핸들 기반 비소유 참조로 기록합니다.
         * @param[in] Actor 충돌 결과에 기록할 액터이며 nullptr이면 참조를 해제합니다.
         */
        void SetActor(Engine::AActor* Actor);

        /**
         * @brief 충돌 결과의 프리미티브 컴포넌트를 비소유 참조로 기록합니다.
         * @param[in] Component 기록할 프리미티브 컴포넌트이며 nullptr이면 참조를 해제합니다.
         */
        void SetComponent(Engine::UPrimitiveComponent* Component);

    public:
        /**
         * @brief 충돌한 삼각형의 인덱스입니다.
         */
        Int32 mFaceIndex{-1};

        /**
         * @brief 기존 Lua 충돌 인터페이스와 호환되는 기본값입니다.
         */
        float mTime{1.0f};

        /**
         * @brief 충돌 시 이동 도형의 위치입니다.
         */
        FVector mLocation{};

        /**
         * @brief 표면 위 실제 충돌점입니다.
         */
        FVector mImpactPoint{};

        /**
         * @brief 충돌면의 단위 법선입니다.
         */
        FVector mNormal{};

        /**
         * @brief 적중 표면의 단위 법선입니다.
         */
        FVector mImpactNormal{};
        FVector mNormalImpulse{};

        /**
         * @brief 접촉점의 침투 깊이입니다.
         */
        float mPenetrationDepth{};

        /**
         * @brief 이동을 차단하는 충돌 여부입니다.
         */
        bool mBlockingHit{};

    private:
        /**
         * @brief 적중 컴포넌트의 비소유 세대 핸들입니다.
         */
        CoreUObject::TObjectRef<Engine::UPrimitiveComponent> mHitComponent{};

        /**
         * @brief 적중 액터의 비소유 세대 핸들입니다.
         */
        CoreUObject::TObjectRef<Engine::AActor> mHitActor{};
    };

}
