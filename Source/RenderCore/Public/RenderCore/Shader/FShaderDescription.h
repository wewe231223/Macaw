/**
 * @file FShaderDescription.h
 * @brief 셰이더 경로·진입점·매크로 정규화와 캐시 키 생성에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <d3dcompiler.h>
#include <filesystem>

namespace RenderCore {

    /**
     * @brief 셰이더 전처리 매크로 이름과 치환 값을 보관합니다.
     */
    struct FShaderMacro {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(FShaderMacro);

    public:
        /**
         * @brief 이 타입의 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShaderMacro);

        /**
         * @brief 전처리기에 전달할 셰이더 매크로의 이름입니다.
         */
        Core::FString mName{};
        ReflectProperty(mName, "Name", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 셰이더 전처리 매크로를 치환할 문자열 값입니다.
         */
        Core::FString mValue{"1"};
        ReflectProperty(mValue, "Value", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();
    };

    /**
     * @brief 셰이더의 소스·진입점·프로필·전처리 및 include 설정입니다.
     */
    struct FShaderDescription {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(FShaderDescription);

    public:
        /**
         * @brief 이 타입의 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShaderDescription);

        /**
         * @brief 셰이더 경로와 매크로를 정규화하고 컴파일 설명의 유효성을 검사합니다.
         * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Normalize(Core::FString& Diagnostics);

        /**
         * @brief 경로·진입점·프로필·매크로·컴파일러를 포함한 셰이더 식별 문자열을 반환합니다.
         * @return 컴파일 옵션과 컴파일러 정보를 직렬화한 식별 문자열입니다.
         */
        Core::FString GetIdentity() const;

        /**
         * @brief 정규화한 셰이더 설명에서 사용하는 캐시 키를 반환합니다.
         * @return 정규화한 컴파일 설명의 SHA-256 캐시 키입니다.
         */
        Core::FString GetCacheKey() const;

        /**
         * @brief 컴파일할 최상위 HLSL 소스 파일의 경로입니다.
         */
        std::filesystem::path mSourcePath{};
        ReflectProperty(mSourcePath, "SourcePath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 컴파일할 HLSL 진입 함수의 이름입니다.
         */
        Core::FString mEntryPoint{};
        ReflectProperty(mEntryPoint, "EntryPoint", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief vs_5_0 등 컴파일할 셰이더 단계와 모델을 지정하는 프로필입니다.
         */
        Core::FString mTargetProfile{};
        ReflectProperty(mTargetProfile, "TargetProfile", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 셰이더 컴파일에 적용할 전처리 매크로 목록입니다.
         */
        Core::TArray<RenderCore::FShaderMacro> mMacros{};
        ReflectProperty(mMacros, "Macros", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief include 파일을 탐색할 추가 폴더 목록입니다.
         */
        Core::TArray<std::filesystem::path> mIncludeDirectories{};
        ReflectProperty(mIncludeDirectories, "IncludeDirectories", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief HLSL 컴파일러에 전달할 컴파일 옵션 비트입니다.
         */
        Core::Uint32 mCompileFlags{D3DCOMPILE_ENABLE_STRICTNESS};
        ReflectProperty(mCompileFlags, "CompileFlags", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();
    };

}
