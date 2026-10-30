/**
 * @file FMeshMaterialSlot.cpp
 * @brief 메시 재질 슬롯의 구현입니다.
 * @ingroup Engine
 */
#include "Engine/FMeshMaterialSlot.h"

#include <utility>

namespace Engine {

    Engine::FMeshMaterialSlot::FMeshMaterialSlot(Core::FString Name, Core::FString MaterialPath)
		: mName{std::move(Name)},
		  mMaterialPath{std::move(MaterialPath)} {
    }

    const Core::FString& Engine::FMeshMaterialSlot::GetName() const {
        return mName;
    }

    const Core::FString& Engine::FMeshMaterialSlot::GetMaterialPath() const {
        return mMaterialPath;
    }

}
