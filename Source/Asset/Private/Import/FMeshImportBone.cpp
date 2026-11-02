#include "Asset/Import/FMeshImportBone.h"

#include <utility>

namespace Asset {

    Asset::FMeshImportBone::FMeshImportBone(Core::FString Name, Core::Int32 ParentIndex, const Core::FMatrix& BindPose)
		: mName{std::move(Name)}, mParentIndex{ParentIndex}, mBindPose{BindPose} {
    }

    const Core::FString& Asset::FMeshImportBone::GetName() const {
        return mName;
    }

    Core::Int32 Asset::FMeshImportBone::GetParentIndex() const {
        return mParentIndex;
    }

    const Core::FMatrix& Asset::FMeshImportBone::GetBindPose() const {
        return mBindPose;
    }

}
