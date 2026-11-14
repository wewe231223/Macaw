/**
 * @file FHitProxy.h
 * @brief CPU 피킹 결과의 객체 핸들과 기즈모 축을 선언합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "FHitProxy.generated.h"

namespace Engine {

    /**
     * @brief 피킹 결과가 나타내는 선택 대상의 종류입니다.
     * @ingroup Engine
     * @note 헤더 툴이 이 타입의 네이티브 열거형 메타데이터 접근자를 생성합니다.
     */
    UENUM()
    enum class EHitProxyType : Core::Uint8 {
        /**
         * @brief 마우스 위치에 선택 대상이 없습니다.
         */
        None,
        /**
         * @brief 월드 광선 질의로 찾은 메시 컴포넌트입니다.
         */
        Object,
        /**
         * @brief 선택 액터에 연결된 축, 평면, 화면 또는 균일 스케일 조작 핸들입니다.
         */
        GizmoAxis
    };

    /**
     * @brief CPU 교차 판정으로 찾은 객체 또는 기즈모 축의 선택 정보를 보관합니다.
     * @ingroup Engine
     * @note 객체 포인터를 보관하지 않으며 mObject는 사용 시 편집 컨텍스트를 통해 유효성을 검사해야 합니다.
     */
    struct FHitProxy {
    public:
        /**
         * @brief 선택 대상이 없는 상태, 메시 컴포넌트 또는 기즈모 축을 구분합니다.
         */
        Engine::EHitProxyType mType{Engine::EHitProxyType::None};

        /**
         * @brief Object에서는 메시 컴포넌트, GizmoAxis에서는 선택 액터의 전역 세대 핸들입니다.
         */
        CoreUObject::TObjectRef<CoreUObject::UObject> mObject{};

        /**
         * @brief 기즈모 핸들을 X/Y/Z=0/1/2, XY/XZ/YZ=3/4/5, 화면=6, 균일 스케일=7로 나타내며 객체 피킹에서는 -1입니다.
         */
        Core::Index_t mAxis{-1};
    };

}
