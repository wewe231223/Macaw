/**
 * @file FMeshMaterialSlot.cpp
 * @brief 메시 재질 슬롯의 구현입니다.
 * @ingroup Engine
 */
#include "Engine/FMeshMaterialSlot.h"

#include <utility>

FMeshMaterialSlot::FMeshMaterialSlot(FString Name, FString MaterialPath)
	: mName{std::move(Name)},
	  mMaterialPath{std::move(MaterialPath)} {
}

const FString& FMeshMaterialSlot::GetName() const {
    return mName;
}

const FString& FMeshMaterialSlot::GetMaterialPath() const {
    return mMaterialPath;
}
