/**
 * @file FShaderCompiler.h
 * @brief HLSL 컴파일과 진단 및 include 의존성 수집에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"
#include "RenderCore/Shader/FShaderDescription.h"

namespace RenderCore {

    /**
     * @brief HLSL 컴파일과 진단 및 include 의존성 수집를 담당합니다.
     */
    class FShaderCompiler {
    public:
        /**
         * @brief 정규화한 설명으로 HLSL을 컴파일하고 바이트코드와 의존성을 수집합니다.
         * @param[in] Description 소스 경로·진입점·프로필·매크로 등의 컴파일 설정입니다.
         * @param[out] Compilation 셰이더 바이트코드와 의존성을 포함하는 컴파일 결과입니다.
         * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Compile(const RenderCore::FShaderDescription& Description, RenderCore::FShaderCompilation& Compilation, Core::FString& Diagnostics) const;
    };

}
