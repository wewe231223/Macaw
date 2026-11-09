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
