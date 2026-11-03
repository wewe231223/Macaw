/**
 * @file FMeshMaterialSlot.h
 * @brief 메시 재질 슬롯의 이름과 에셋 경로입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"

namespace Asset {

    /**
     * @brief 임포트한 원본 재질 이름과 Content 기준 재질 경로를 보관합니다.
     */
    class FMeshMaterialSlot {
    public:
        /**
         * @brief 비어 있는 재질 슬롯을 구성합니다.
         */
        FMeshMaterialSlot() = default;

        /**
         * @brief 재질 슬롯을 구성합니다.
         * @param Name 원본 재질 이름입니다.
         * @param MaterialPath Content 기준 재질 경로입니다.
         */
        FMeshMaterialSlot(Core::FString Name, Core::FString MaterialPath);

    public:
        /**
         * @brief 슬롯 이름을 반환합니다.
         * @return 원본 재질 이름의 참조입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 재질 경로를 반환합니다.
         * @return Content 기준 재질 경로의 참조입니다.
         */
        const Core::FString& GetMaterialPath() const;

    private:
        /**
         * @brief 원본 파일에 기록된 재질 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief Content 기준 .masset 재질 에셋 경로이며 빈 경로는 기본 재질입니다.
         */
        Core::FString mMaterialPath{};
    };

}
