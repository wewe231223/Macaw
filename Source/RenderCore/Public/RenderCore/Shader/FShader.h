/**
 * @file FShader.h
 * @brief 셰이더 설명과 공유 컴파일 결과 및 갱신 세대 관리에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"
#include "RenderCore/Shader/FShaderDescription.h"

#include <span>

class FShaderManager;

/**
 * @brief 셰이더 설명과 공유 컴파일 결과 및 갱신 세대 관리를 담당합니다.
 */
class FShader {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FShader);

public:
    /**
     * @brief 전달받은 설정과 의존성으로 FShader의 초기 상태를 구성합니다.
     * @param[in] Description 소스 경로·진입점·프로필·매크로 등의 컴파일 설정입니다.
     * @param[in] CacheKey 캐시 파일을 구분하는 셰이더 해시 키입니다.
     * @param[in] Compilation 셰이더 바이트코드와 의존성을 포함하는 컴파일 결과입니다.
     * @param[in] LoadedFromCache 컴파일 결과가 디스크 캐시에서 읽힌 것인지 나타냅니다.
     */
    FShader(FShaderDescription Description, FString CacheKey, FShaderCompilation Compilation, bool LoadedFromCache);
    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    ~FShader() = default;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FShader(const FShader&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FShader& operator=(const FShader&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FShader(FShader&&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FShader& operator=(FShader&&) = delete;

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FShader);
    /**
     * @brief 셰이더의 정규화된 컴파일 설명을 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const FShaderDescription& GetDescription() const;
    /**
     * @brief 정규화한 셰이더 설명에서 사용하는 캐시 키를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const FString& GetCacheKey() const;
    /**
     * @brief 현재 컴파일 결과의 셰이더 바이트코드를 비소유 뷰로 반환합니다.
     * @return 현재 컴파일 결과가 살아 있는 동안 유효한 읽기 전용 바이트코드 뷰입니다.
     */
    std::span<const Uint8> GetByteCode() const;
    /**
     * @brief 호출 후에도 현재 컴파일 결과의 수명을 유지할 공유 참조를 반환합니다.
     * @return 현재 컴파일 결과의 수명을 유지하는 공유 참조입니다.
     */
    std::shared_ptr<const FShaderCompilation> GetCompilation() const;
    /**
     * @brief 컴파일 결과를 교체할 때 증가하는 셰이더 갱신 세대를 반환합니다.
     * @return 컴파일 결과를 교체할 때 증가하는 셰이더 갱신 세대를 나타내는 정수 값입니다.
     */
    Uint64 GetRevision() const;
    /**
     * @brief 현재 컴파일 결과를 디스크 캐시에서 읽었는지 반환합니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    bool IsLoadedFromCache() const;

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
    /**
     * @brief 셰이더 관리자가 컴파일 결과를 교체하도록 접근을 허용합니다.
     */
    friend class FShaderManager;

    /**
     * @brief 새 컴파일 결과를 공유 스냅샷으로 교체하고 갱신 세대를 증가시킵니다.
     * @param[in] Compilation 셰이더 바이트코드와 의존성을 포함하는 컴파일 결과입니다.
     * @param[in] LoadedFromCache 컴파일 결과가 디스크 캐시에서 읽힌 것인지 나타냅니다.
     */
    void Commit(FShaderCompilation Compilation, bool LoadedFromCache);

private:
    /**
     * @brief 셰이더 컴파일 경로와 진입점·프로필·매크로 설정입니다.
     */
    FShaderDescription mDescription{};
    ReflectProperty(mDescription, "Description", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 셰이더 설명과 컴파일러 정보를 구분하는 캐시 키입니다.
     */
    FString mCacheKey{};
    ReflectProperty(mCacheKey, "CacheKey", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 이전에 받은 참조의 수명도 유지하는 불변 컴파일 결과입니다.
     */
    std::shared_ptr<const FShaderCompilation> mCompilation{};
    ReflectProperty(mCompilation, "Compilation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 성공한 컴파일 결과 교체마다 증가하는 갱신 세대입니다.
     */
    Uint64 mRevision{1};
    ReflectProperty(mRevision, "Revision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 현재 바이트코드를 디스크 캐시에서 불러왔는지 나타냅니다.
     */
    bool mLoadedFromCache{};
    ReflectProperty(mLoadedFromCache, "LoadedFromCache", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
