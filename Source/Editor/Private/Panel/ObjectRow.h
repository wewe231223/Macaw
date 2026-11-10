/**
 * @file ObjectRow.h
 * @brief 아웃라이너와 디테일 패널이 공유하는 객체 테이블 행 표시 함수를 선언합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"

namespace CoreUObject {
    class UObject;
}

namespace Editor {

    /**
     * @brief 액터와 컴포넌트를 아이콘, 이름, 타입 컬럼의 테이블 행으로 표시합니다.
     * @ingroup Editor
     */
    namespace ObjectRow {
        /**
         * @brief 클래스 접두사를 뺀 타입 이름을 만듭니다.
         * @param[in] Object 타입을 표시할 객체입니다.
         * @return AStaticMeshActor이면 StaticMeshActor처럼 접두사를 제거한 이름입니다.
         */
        Core::FString GetTypeLabel(const CoreUObject::UObject* Object);

        /**
         * @brief 현재 행의 다음 컬럼에 종류별 아이콘과 이름을 표시합니다.
         * @param[in] Object 표시할 액터 또는 컴포넌트입니다.
         * @param[in] Selected 행을 선택 색상으로 강조할지 지정합니다.
         * @note 행은 호출자가 ImGui::TableNextRow()로 시작합니다. 클릭 검사는 호출 직후 ImGui::IsItemClicked()로 수행합니다.
         */
        void DrawLabel(CoreUObject::UObject* Object, bool Selected, float Indent = 0.0f);

        /**
         * @brief 현재 행의 다음 컬럼에 타입 이름을 표시합니다.
         * @param[in] Object 타입을 표시할 객체입니다.
         */
        void DrawType(const CoreUObject::UObject* Object);
    }

}
