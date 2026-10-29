/**
 * @file FAssetManager.h
 * @brief 프로젝트 콘텐츠 경로와 메시·텍스처 에셋의 로드 및 캐시에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>
#include <memory>

class UStaticMesh;
class UTexture2D;
class UMaterial;

/**
 * @brief 프로젝트 콘텐츠 경로와 메시·텍스처 에셋의 로드 및 캐시를 담당합니다.
 */
class FAssetManager {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FAssetManager);

public:
    /**
     * @brief 전달받은 설정과 의존성으로 FAssetManager의 초기 상태를 구성합니다.
     * @param[in] ContentRoot 프로젝트 에셋 경로의 기준 콘텐츠 폴더입니다.
     */
    explicit FAssetManager(std::filesystem::path ContentRoot);
    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    ~FAssetManager();
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FAssetManager(const FAssetManager&) = delete;
    /**
     * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
     */
    FAssetManager& operator=(const FAssetManager&) = delete;

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FAssetManager);
    /**
     * @brief 에셋 경로의 기준이 되는 콘텐츠 폴더를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const std::filesystem::path& GetContentRoot() const;
    /** @brief 이 관리자가 보유한 메시의 경로를 조회합니다. @param Mesh 조회할 메시의 비소유 주소입니다. @return Content 기준 경로이며 관리하지 않는 메시이면 빈 문자열입니다. */
    FString GetMeshPath(const UStaticMesh* Mesh) const;
    /**
     * @brief 이 관리자의 캐시에서 텍스처에 대응하는 에셋 경로를 조회합니다.
     * @param[in] Texture 경로를 조회할 텍스처의 비소유 주소입니다.
     * @return 정규화한 Content 상대 경로이며 관리하지 않는 텍스처이면 빈 문자열입니다.
     */
    FString GetTexturePath(const UTexture2D* Texture) const;
    /**
     * @brief 콘텐츠 폴더를 벗어나는 경로를 거부하고 정규화된 절대 경로를 반환합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 검증한 콘텐츠 내부 경로이며 실패하면 비어 있습니다.
     */
    std::filesystem::path ResolvePath(const std::filesystem::path& Path, FString& Error) const;
    /**
     * @brief 경로가 콘텐츠 폴더 안에 있는지 검증하고 UTF-8 상대 경로를 반환합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 검증한 콘텐츠 내부 경로이며 실패하면 비어 있습니다.
     */
    FString GetRelativePath(const std::filesystem::path& Path, FString& Error) const;
    /**
     * @brief 콘텐츠 폴더 아래의 .masset 파일을 찾아 상대 경로순으로 반환합니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 정렬한 콘텐츠 상대 에셋 경로 목록이며 실패 시 Error에 원인을 기록합니다.
     */
    TArray<std::filesystem::path> GetAssetPaths(FString& Error) const;
    /**
     * @brief 메시 에셋을 캐시에서 조회하거나 파일에서 로드하여 보관합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    UStaticMesh* LoadMesh(const std::filesystem::path& Path, FString& Error);
    /**
     * @brief 텍스처 에셋을 캐시에서 조회하거나 파일에서 로드하여 보관합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    UTexture2D* LoadTexture(const std::filesystem::path& Path, FString& Error);
    /** @brief MTL 머티리얼과 텍스처 참조를 로드하여 캐시에 보관합니다. @param[in] Path 프로젝트 머티리얼 경로입니다. @param[out] Error 실패 원인입니다. @return 캐시 소유 머티리얼의 비소유 주소이며 실패하면 nullptr입니다. */
    UMaterial* LoadMaterial(const std::filesystem::path& Path, FString& Error);
    /** @brief 캐시 객체의 주소를 유지하면서 파일의 최신 설정을 다시 적용합니다. @param[in] Path 다시 읽을 프로젝트 머티리얼 경로입니다. @param[out] Error 실패 원인입니다. @return 설정과 참조 갱신에 성공하면 true입니다. */
    bool ReloadMaterial(const std::filesystem::path& Path, FString& Error);
    /** @brief 머티리얼을 안전하게 저장하고 같은 경로의 캐시 객체를 갱신합니다. @param[in] Path 저장할 프로젝트 머티리얼 경로입니다. @param[in] Material 저장할 설정입니다. @param[out] Error 저장 또는 캐시 갱신의 실패 원인입니다. @return 저장과 캐시 갱신에 성공하면 true입니다. */
    bool SaveMaterial(const std::filesystem::path& Path, const UMaterial& Material, FString& Error);
    /**
     * @brief 검증한 CPU 메시 데이터를 바이너리 에셋 파일로 저장합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[in] Mesh 읽거나 직렬화할 메시 데이터입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool SaveMesh(const std::filesystem::path& Path, const UStaticMesh& Mesh, FString& Error);
    /**
     * @brief 텍스처의 크기·형식·색 공간과 이미지 데이터를 에셋 파일로 저장합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[in] Texture 데이터를 읽거나 뷰를 생성할 텍스처이며 소유권을 이전하지 않습니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool SaveTexture(const std::filesystem::path& Path, const UTexture2D& Texture, FString& Error);
    /**
     * @brief 외부 메시를 읽어 프로젝트의 메시 에셋 파일로 저장합니다.
     * @param[in] Source 프로젝트에 가져올 외부 파일 경로입니다.
     * @param[in] Target 프로젝트 콘텐츠 폴더 안에 저장할 대상 에셋 경로입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool ImportMesh(const std::filesystem::path& Source, const std::filesystem::path& Target, FString& Error);
    /**
     * @brief 외부 이미지를 읽어 프로젝트의 텍스처 에셋 파일로 저장합니다.
     * @param[in] Source 프로젝트에 가져올 외부 파일 경로입니다.
     * @param[in] Target 프로젝트 콘텐츠 폴더 안에 저장할 대상 에셋 경로입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool ImportTexture(const std::filesystem::path& Source, const std::filesystem::path& Target, FString& Error);

private:
    /** @brief 캐시 객체의 주소를 유지하면서 머티리얼 설정과 텍스처 참조를 갱신합니다. @param[in,out] Target 설정을 적용할 기존 객체입니다. @param[in] Source 복사할 설정을 제공하는 객체입니다. @param[out] Error 텍스처 참조를 해석할 수 없는 경우의 원인입니다. @return 텍스처 참조까지 복원하면 true입니다. */
    bool ApplyMaterialSettings(UMaterial& Target, const UMaterial& Source, FString& Error);

    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 에셋 접근을 제한하는 프로젝트 콘텐츠 폴더의 절대 경로입니다.
     */
    std::filesystem::path mContentRoot{};
    ReflectProperty(mContentRoot, "ContentRoot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 정규화한 상대 경로별로 소유하는 메시 에셋 캐시입니다.
     */
    TMap<FString, std::unique_ptr<UStaticMesh>> mMeshes{};
    ReflectProperty(mMeshes, "Meshes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 정규화한 상대 경로별로 소유하는 텍스처 에셋 캐시입니다.
     */
    TMap<FString, std::unique_ptr<UTexture2D>> mTextures{};
    ReflectProperty(mTextures, "Textures", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 정규화한 상대 경로별로 소유하는 머티리얼 캐시입니다. */
    TMap<FString, std::unique_ptr<UMaterial>> mMaterials{};
    ReflectProperty(mMaterials, "Materials", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
