/**
 * @file FShaderManager.h
 * @brief 셰이더 캐시 로드와 변경 감지 및 재컴파일에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/Shader/FShader.h"
#include "RenderCore/Shader/FShaderCache.h"
#include "RenderCore/Shader/FShaderCompiler.h"

/**
 * @brief 셰이더 캐시 로드와 변경 감지 및 재컴파일를 담당합니다.
 */
class FShaderManager {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FShaderManager);

public:
    /**
     * @brief 전달받은 설정과 의존성으로 FShaderManager의 초기 상태를 구성합니다.
     * @param[in] CacheDirectory 컴파일된 셰이더의 캐시 파일을 보관할 폴더입니다.
     */
    explicit FShaderManager(std::filesystem::path CacheDirectory = "Intermediate/ShaderCache");
    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    ~FShaderManager() = default;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FShaderManager(const FShaderManager&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FShaderManager& operator=(const FShaderManager&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FShaderManager(FShaderManager&&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FShaderManager& operator=(FShaderManager&&) = delete;

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FShaderManager);
    /**
     * @brief 셰이더를 메모리 캐시에서 조회하거나 디스크 캐시·컴파일로 준비합니다.
     * @param[in] Description 소스 경로·진입점·프로필·매크로 등의 컴파일 설정입니다.
     * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
     * @return 요청한 텍스처 형식 또는 사용 용도 값입니다.
     */
    std::shared_ptr<const FShader> Load(const FShaderDescription& Description, FString& Diagnostics);
    /**
     * @brief 셰이더의 의존성 스냅샷과 파일 상태를 비교하여 변경을 감지합니다.
     * @param[in] Shader 소스 파일 변경 여부를 검사할 셰이더입니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    bool HasChanges(const FShader& Shader) const;
    /**
     * @brief 변경된 셰이더를 재컴파일하고 갱신에 성공한 셰이더를 모읍니다.
     * @param[out] ChangedShaders 컴파일 결과가 갱신된 셰이더를 받을 목록입니다.
     * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool ReloadChanged(TArray<std::shared_ptr<const FShader>>& ChangedShaders, FString& Diagnostics);
    /**
     * @brief 관리 중인 셰이더의 공유 참조를 모두 제거합니다.
     */
    void Clear();

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
    /**
     * @brief 유효한 캐시를 먼저 조회하고 사용할 수 없으면 셰이더를 컴파일합니다.
     * @param[in] Description 소스 경로·진입점·프로필·매크로 등의 컴파일 설정입니다.
     * @param[out] Compilation 셰이더 바이트코드와 의존성을 포함하는 컴파일 결과입니다.
     * @param[out] LoadedFromCache 컴파일 결과가 디스크 캐시에서 읽힌 것인지 나타냅니다.
     * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool LoadCompilation(const FShaderDescription& Description, FShaderCompilation& Compilation, bool& LoadedFromCache, FString& Diagnostics) const;

private:
    /**
     * @brief HLSL 컴파일과 의존성 수집을 수행하는 컴파일러입니다.
     */
    FShaderCompiler mCompiler{};
    ReflectProperty(mCompiler, "Compiler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 셰이더 컴파일 결과의 디스크 캐시입니다.
     */
    FShaderCache mCache{};
    ReflectProperty(mCache, "Cache", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 정규화한 식별 문자열별로 공유 소유하는 셰이더 목록입니다.
     */
    TMap<FString, std::shared_ptr<FShader>> mShaders{};
    ReflectProperty(mShaders, "Shaders", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
