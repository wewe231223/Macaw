/**
 * @file CodeGenerator.h
 * @brief 파싱한 헤더로 generated.h, 모듈 리플렉션 등록 코드와 C# 바인딩을 만듭니다.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "HeaderParser.h"

namespace HeaderTool {

    /**
     * @brief 작업 공간의 모든 헤더입니다.
     */
    struct FWorkspace {
        /**
         * @brief 작업 공간 루트 경로입니다.
         */
        std::string Root;

        /**
         * @brief 경로 순으로 정렬한 헤더입니다.
         */
        std::vector<std::unique_ptr<FHeader>> Headers;
    };

    /**
     * @brief 모듈의 생성 디렉터리를 작업 공간 기준으로 반환합니다.
     * @param[in] Module Source 아래 디렉터리 이름입니다. App은 Macaw 디렉터리를 씁니다.
     * @return 생성 디렉터리 경로입니다.
     */
    std::string GetGeneratedDirectory(const std::string& Module);

    /**
     * @brief 헤더의 타입을 등록하는 모듈 이름을 반환합니다. Core 타입은 CoreUObject가 등록합니다.
     * @param[in] SourceModule Source 아래 디렉터리 이름입니다.
     * @return 등록 모듈 이름입니다.
     */
    std::string GetRegistrationModule(const std::string& SourceModule);

    /**
     * @brief 모듈 디렉터리에 있는 헤더의 generated.h를 씁니다.
     * @param[in] Workspace 파싱한 작업 공간입니다.
     * @param[in] SourceModule Source 아래 디렉터리 이름입니다.
     */
    void WriteGeneratedHeaders(const FWorkspace& Workspace, const std::string& SourceModule);

    /**
     * @brief 모듈의 리플렉션 등록 헤더와 소스를 씁니다.
     * @param[in] Workspace 파싱한 작업 공간입니다.
     * @param[in] Module 등록 모듈 이름입니다.
     */
    void WriteModuleReflection(const FWorkspace& Workspace, const std::string& Module);

    /**
     * @brief Script 모듈의 네이티브 썽크와 Macaw.Engine의 C# 래퍼를 씁니다.
     * @param[in] Workspace 파싱한 작업 공간입니다.
     */
    void WriteBindings(const FWorkspace& Workspace);

    /**
     * @brief 내용이 바뀐 경우에만 CRLF 줄바꿈으로 파일을 씁니다.
     * @param[in] Root 작업 공간 루트입니다.
     * @param[in] Path 작업 공간 기준 경로입니다.
     * @param[in] Lines 쓸 줄입니다.
     */
    void WriteIfChanged(const std::string& Root, const std::string& Path, const std::vector<std::string>& Lines);

}
