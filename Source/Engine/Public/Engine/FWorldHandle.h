/**
 * @file FWorldHandle.h
 * @brief 월드 슬롯과 세대를 결합하는 비소유 식별자를 정의합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Engine {

    /**
     * @brief 엔진이 소유한 월드를 슬롯과 세대로 식별하는 비소유 핸들입니다.
     * @ingroup Engine
     * @note 세대는 프로세스 전체에서 발급되어 다른 엔진이나 재사용된 슬롯의 월드와 구분됩니다. 실제 생존 여부는
     * UEngine::GetWorld(FWorldHandle)로 확인합니다.
     */
    class FWorldHandle {
    public:
        /**
         * @brief 어떤 월드도 가리키지 않는 무효 핸들을 구성합니다.
         */
        FWorldHandle() = default;

        /**
         * @brief 슬롯과 세대를 조합하여 핸들을 구성합니다.
         * @param[in] Index 엔진 컨텍스트 배열의 슬롯입니다.
         * @param[in] Generation 해당 슬롯에 발급된 월드 세대입니다.
         */
        FWorldHandle(Core::Int32 Index, Core::Uint64 Generation);

    public:
        /**
         * @brief 슬롯과 세대가 무효 기본값이 아닌지 검사합니다.
         * @return 핸들 형식이 유효하면 true입니다.
         * @note 월드의 생존 여부나 특정 엔진의 소유 여부는 검사하지 않습니다.
         */
        bool IsValid() const;

        /**
         * @brief 월드 컨텍스트의 슬롯을 반환합니다.
         * @return 엔진 컨텍스트 배열의 인덱스입니다.
         */
        Core::Int32 GetIndex() const;

        /**
         * @brief 월드 슬롯의 재사용을 구분하는 세대를 반환합니다.
         * @return 발급된 세대이며 기본 무효 핸들은 0입니다.
         */
        Core::Uint64 GetGeneration() const;

        /**
         * @brief 슬롯과 세대가 모두 같은지 비교합니다.
         * @param[in] Other 비교할 핸들입니다.
         * @return 두 식별 값이 모두 같으면 true입니다.
         */
        bool operator==(const Engine::FWorldHandle& Other) const;

    private:
        /**
         * @brief 엔진의 월드 슬롯이며 최댓값은 무효 슬롯입니다.
         */
        Core::Int32 mIndex{-1};

        /**
         * @brief 월드 슬롯에 발급된 세대이며 0은 무효 세대입니다.
         */
        Core::Uint64 mGeneration{};
    };

}
