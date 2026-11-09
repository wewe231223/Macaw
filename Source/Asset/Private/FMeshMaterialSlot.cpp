/**
 * @file FMeshMaterialSlot.cpp
 * @brief 메시 재질 슬롯의 구현입니다.
 * @ingroup Asset
 */

#include "Asset/FMeshMaterialSlot.h"

#include <utility>

namespace Asset {

    /**
     * @brief 재질 슬롯을 구성합니다.
     */
    Asset::FMeshMaterialSlot::FMeshMaterialSlot(Core::FString Name, Core::FString MaterialPath)
		: mName{std::move(Name)},
		  mMaterialPath{std::move(MaterialPath)} {
    }

    /**
     * @brief 슬롯 이름을 반환합니다.
     */
    const Core::FString& Asset::FMeshMaterialSlot::GetName() const {
        return mName;
    }

    /**
     * @brief 재질 경로를 반환합니다.
     */
    const Core::FString& Asset::FMeshMaterialSlot::GetMaterialPath() const {
        return mMaterialPath;
    }

}
