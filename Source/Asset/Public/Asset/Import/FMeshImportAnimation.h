/**
 * @file FMeshImportAnimation.h
 * @brief 원본 파일의 애니메이션 스택 하나를 재샘플링한 결과입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Animation/FAnimDataModel.h"

namespace Asset {

    /**
     * @brief 애니메이션 스택 이름과 재샘플링한 본 트랙입니다.
     */
    class FMeshImportAnimation {
    public:

        /**
         * @brief FMeshImportAnimation의 초기 상태와 의존성을 구성합니다.
         */
        FMeshImportAnimation() = default;

        /**
         * @brief 스택 이름과 데이터를 지정해 생성합니다.
         * @param[in] Name 원본 애니메이션 스택 이름입니다.
         * @param[in] DataModel 재샘플링한 트랙과 시간 정보입니다.
         */
        FMeshImportAnimation(Core::FString Name, Asset::FAnimDataModel DataModel);

    public:

        /**
         * @brief 원본 애니메이션 스택 이름을 반환합니다.
         * @return 스택 이름입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 재샘플링한 데이터를 반환합니다.
         * @return 트랙과 시간 정보입니다.
         */
        const Asset::FAnimDataModel& GetDataModel() const;

        /**
         * @brief 재샘플링한 데이터를 반환합니다.
         * @return 이동할 수 있는 트랙과 시간 정보입니다.
         */
        Asset::FAnimDataModel& GetDataModel();

    private:

        /**
         * @brief 원본 애니메이션 스택 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 재샘플링한 트랙과 시간 정보입니다.
         */
        Asset::FAnimDataModel mDataModel{};
    };

}
