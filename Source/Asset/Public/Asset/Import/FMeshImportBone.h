/**
 * @file FMeshImportBone.h
 * @brief 원본 스켈레톤 계층에서 읽은 본의 이름·부모·부모 본 기준 바인드 포즈입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Core/Math/FMatrix.h"

namespace Asset {

    /**
     * @brief 원본 스켈레톤 계층에서 읽은 본의 이름·부모·부모 본 기준 바인드 포즈입니다.
     * @ingroup Asset
     */
    class FMeshImportBone {
    public:
        /**
         * @brief 빈 본을 생성합니다.
         */
        FMeshImportBone() = default;

        /**
         * @brief 본의 이름·부모·바인드 포즈를 지정해 생성합니다.
         * @param[in] Name 원본 본 이름입니다.
         * @param[in] ParentIndex 결과 본 목록에서 부모 본의 인덱스이며 루트 본이면 -1입니다.
         * @param[in] BindPose 부모 본 공간 기준의 로컬 바인드 포즈이며 루트 본은 엔진 공간 기준입니다.
         */
        FMeshImportBone(Core::FString Name, Core::Index_t ParentIndex, const Core::FMatrix& BindPose);

    public:
        /**
         * @brief 본 이름을 조회합니다.
         * @return 원본 본 이름의 읽기 전용 참조입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 부모 본 인덱스를 조회합니다.
         * @return 결과 본 목록에서 부모 본의 인덱스이며 루트 본이면 -1입니다.
         */
        Core::Index_t GetParentIndex() const;

        /**
         * @brief 바인드 포즈를 조회합니다.
         * @return 부모 본 공간 기준의 로컬 바인드 포즈이며 루트 본은 엔진 공간 기준입니다.
         */
        const Core::FMatrix& GetBindPose() const;

    private:
        /**
         * @brief 원본 본 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 결과 본 목록에서 부모 본의 인덱스이며 루트 본이면 -1입니다.
         */
        Core::Index_t mParentIndex{-1};

        /**
         * @brief 부모 본 공간 기준의 로컬 바인드 포즈이며 루트 본은 엔진 공간 기준입니다. 전역 포즈는 부모 쪽으로 곱해 구합니다.
         */
        Core::FMatrix mBindPose{};
    };

}
