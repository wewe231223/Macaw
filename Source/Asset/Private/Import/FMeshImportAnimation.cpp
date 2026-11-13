/**
 * @file FMeshImportAnimation.cpp
 * @brief 임포트한 애니메이션 트랙과 시퀀스 메타데이터를 보관합니다.
 * @ingroup Asset
 */

#include "Asset/Import/FMeshImportAnimation.h"

#include <utility>

namespace Asset {

    Asset::FMeshImportAnimation::FMeshImportAnimation(Core::FString Name, Asset::FAnimDataModel DataModel)
        : mName{std::move(Name)}, mDataModel{std::move(DataModel)} {
    }

    const Core::FString& Asset::FMeshImportAnimation::GetName() const {
        return mName;
    }

    const Asset::FAnimDataModel& Asset::FMeshImportAnimation::GetDataModel() const {
        return mDataModel;
    }

    Asset::FAnimDataModel& Asset::FMeshImportAnimation::GetDataModel() {
        return mDataModel;
    }

}
