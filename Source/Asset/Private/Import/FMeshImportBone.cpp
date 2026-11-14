/**
 * @file FMeshImportBone.cpp
 * @brief 원본 스켈레톤 계층에서 읽은 본의 이름·부모·부모 본 기준 바인드 포즈입니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
#include "Asset/Import/FMeshImportBone.h"

#include <utility>

namespace Asset {

    /**
     * @brief 본의 이름·부모·바인드 포즈를 지정해 생성합니다.
     */
    Asset::FMeshImportBone::FMeshImportBone(Core::FString Name, Core::Index_t ParentIndex, const Core::FMatrix& BindPose)
		: mName{std::move(Name)}, mParentIndex{ParentIndex}, mBindPose{BindPose} {
    }

    /**
     * @brief 본 이름을 조회합니다.
     */
    const Core::FString& Asset::FMeshImportBone::GetName() const {
        return mName;
    }

    /**
     * @brief 부모 본 인덱스를 조회합니다.
     */
    Core::Index_t Asset::FMeshImportBone::GetParentIndex() const {
        return mParentIndex;
    }

    /**
     * @brief 바인드 포즈를 조회합니다.
     */
    const Core::FMatrix& Asset::FMeshImportBone::GetBindPose() const {
        return mBindPose;
    }

}
