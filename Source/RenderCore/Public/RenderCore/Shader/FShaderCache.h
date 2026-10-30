/**
 * @file FShaderCache.h
 * @brief 컴파일된 셰이더의 디스크 캐시와 의존성 유효성 검사에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"
#include "RenderCore/Shader/FShaderDescription.h"

namespace RenderCore {

    /**
     * @brief 컴파일된 셰이더의 디스크 캐시와 의존성 유효성 검사를 담당합니다.
     */
    class FShaderCache {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(FShaderCache);

    public:
        /**
         * @brief 전달받은 설정과 의존성으로 FShaderCache의 초기 상태를 구성합니다.
         * @param[in] Directory 이후 셰이더 캐시에 사용할 폴더 경로입니다.
         */
        explicit FShaderCache(std::filesystem::path Directory = "Intermediate/ShaderCache");

    public:
        /**
         * @brief 이 타입의 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShaderCache);

        /**
         * @brief 캐시 키에 대응하는 셰이더 캐시 파일 경로를 반환합니다.
         * @param[in] CacheKey 캐시 파일을 구분하는 셰이더 해시 키입니다.
         * @return 요청하거나 변환한 파일 시스템 경로 또는 시각입니다.
         */
        std::filesystem::path GetPath(const Core::FString& CacheKey) const;

        /**
         * @brief 캐시의 체크섬·셰이더 식별자·의존성을 검증하여 컴파일 결과를 읽습니다.
         * @param[in] Description 소스 경로·진입점·프로필·매크로 등의 컴파일 설정입니다.
         * @param[out] Compilation 셰이더 바이트코드와 의존성을 포함하는 컴파일 결과입니다.
         * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Load(const RenderCore::FShaderDescription& Description, RenderCore::FShaderCompilation& Compilation, Core::FString& Diagnostics) const;

        /**
         * @brief 컴파일 결과와 의존성 및 체크섬을 캐시 파일에 저장합니다.
         * @param[in] Description 소스 경로·진입점·프로필·매크로 등의 컴파일 설정입니다.
         * @param[in] Compilation 셰이더 바이트코드와 의존성을 포함하는 컴파일 결과입니다.
         * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Save(const RenderCore::FShaderDescription& Description, const RenderCore::FShaderCompilation& Compilation, Core::FString& Diagnostics) const;

        /**
         * @brief 컴파일 결과의 소스·include 의존성이 디스크의 현재 상태와 일치하는지 검사합니다.
         * @param[in] Compilation 셰이더 바이트코드와 의존성을 포함하는 컴파일 결과입니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsCurrent(const RenderCore::FShaderCompilation& Compilation) const;

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 컴파일된 셰이더의 캐시 파일을 읽고 쓰는 폴더입니다.
         */
        std::filesystem::path mDirectory{};
        ReflectProperty(mDirectory, "Directory", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
