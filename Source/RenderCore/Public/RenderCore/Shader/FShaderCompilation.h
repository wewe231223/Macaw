/**
 * @file FShaderCompilation.h
 * @brief 컴파일된 셰이더 바이트코드와 소스 파일 의존성에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

namespace RenderCore {

    /**
     * @brief 의존 파일의 경로와 존재 여부·크기·수정 시각을 보관합니다.
     */
    struct FShaderDependency {
    public:
        /**
         * @brief 의존 파일의 정규화된 경로입니다.
         */
        std::filesystem::path mPath{};

        /**
         * @brief 의존성 검사 시 파일의 마지막 수정 시각입니다.
         */
        std::filesystem::file_time_type mWriteTime{};

        /**
         * @brief 의존성 검사 시 파일의 바이트 수입니다.
         */
        Core::Uint64 mSize{};

        /**
         * @brief 의존성 검사 시 파일이 존재했는지 나타냅니다.
         */
        bool mExists{};
    };

    /**
     * @brief 컴파일 결과의 바이트코드와 의존성 및 생성 시각을 보관합니다.
     */
    struct FShaderCompilation {
    public:
        /**
         * @brief 컴파일된 셰이더 바이트코드입니다.
         */
        Core::TArray<Core::Uint8> mByteCode{};

        /**
         * @brief 소스 및 include 파일의 존재·크기·수정 시각 스냅샷입니다.
         */
        Core::TArray<RenderCore::FShaderDependency> mDependencies{};

        /**
         * @brief 셰이더 컴파일 결과를 생성한 시각입니다.
         */
        std::filesystem::file_time_type mBuildTime{};
    };

}
