/**
 * @file FMeshMaterialSlot.cpp
 * @brief 메시 재질 슬롯의 구현입니다.
 * @ingroup Asset
 */
#include "Asset/FMeshMaterialSlot.h"

#include <utility>

namespace Asset {

    Asset::FMeshMaterialSlot::FMeshMaterialSlot(Core::FString Name, Core::FString MaterialPath)
		: mName{std::move(Name)},
		  mMaterialPath{std::move(MaterialPath)} {
    }

    const Core::FString& Asset::FMeshMaterialSlot::GetName() const {
        return mName;
    }

    const Core::FString& Asset::FMeshMaterialSlot::GetMaterialPath() const {
        return mMaterialPath;
    }

}
