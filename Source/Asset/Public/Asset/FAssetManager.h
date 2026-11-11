/**
 * @file FAssetManager.h
 * @brief 프로젝트 콘텐츠 경로와 메시·텍스처 에셋의 로드 및 캐시에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/FAssetFile.h"
#include "Asset/Import/FAssetImportOption.h"
#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>
#include <memory>

namespace CoreUObject {

    class UObject;

}

namespace Asset {

    class UStaticMesh;
    class USkeletalMesh;
    class USkeleton;
    class UTexture2D;
    class UMaterial;
    class ULevelSequence;
    class UAnimSequence;
    class UBlendSpace;
    class UAnimGraph;

    /**
     * @brief 프로젝트 콘텐츠 경로와 메시·텍스처 에셋의 로드 및 캐시를 담당합니다.
     */
    class FAssetManager {
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
         * @brief 소유 상태가 중복되지 않도록 복사·이동 생성을 금지합니다.
         */
        FAssetManager(const Asset::FAssetManager&) = delete;

        /**
         * @brief 소유 상태가 중복되지 않도록 대입을 금지합니다.
         */
        Asset::FAssetManager& operator=(const Asset::FAssetManager&) = delete;

    public:
        /**
         * @brief 에셋 경로의 기준이 되는 콘텐츠 폴더를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const std::filesystem::path& GetContentRoot() const;

        /**
         * @brief 이 관리자가 보유한 메시의 경로를 조회합니다.
         * @param Mesh 조회할 메시의 비소유 주소입니다.
         * @return Content 기준 경로이며 관리하지 않는 메시이면 빈 문자열입니다.
         */
        Core::FString GetMeshPath(const Asset::UStaticMesh* Mesh) const;

        /**
         * @brief 캐시에 등록된 스켈레탈 메시의 프로젝트 상대 경로를 반환합니다.
         * @param[in] Mesh 조회·저장·포즈 계산에 사용할 메시입니다.
         * @return 캐시 경로이며 등록하지 않은 메시이면 빈 문자열입니다.
         */
        Core::FString GetSkeletalMeshPath(const Asset::USkeletalMesh* Mesh) const;

        /**
         * @brief 이 관리자의 캐시에서 텍스처에 대응하는 에셋 경로를 조회합니다.
         * @param[in] Texture 경로를 조회할 텍스처의 비소유 주소입니다.
         * @return 정규화한 Content 상대 경로이며 관리하지 않는 텍스처이면 빈 문자열입니다.
         */
        Core::FString GetTexturePath(const Asset::UTexture2D* Texture) const;

        /**
         * @brief 콘텐츠 폴더를 벗어나는 경로를 거부하고 정규화된 절대 경로를 반환합니다.
         * @param[in] Path Content 기준 상대 경로 또는 절대 경로입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 검증한 콘텐츠 내부 경로이며 실패하면 비어 있습니다.
         */
        std::filesystem::path ResolvePath(const std::filesystem::path& Path, Core::FString& Error) const;

        /**
         * @brief 경로가 콘텐츠 폴더 안에 있는지 검증하고 UTF-8 상대 경로를 반환합니다.
         * @param[in] Path Content 기준 상대 경로 또는 절대 경로입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 검증한 콘텐츠 내부 경로이며 실패하면 비어 있습니다.
         */
        Core::FString GetRelativePath(const std::filesystem::path& Path, Core::FString& Error) const;

        /**
         * @brief 콘텐츠 폴더 아래의 .masset 파일을 찾아 상대 경로순으로 반환합니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 정렬한 콘텐츠 상대 에셋 경로 목록이며 실패 시 Error에 원인을 기록합니다.
         */
        Core::TArray<std::filesystem::path> GetAssetPaths(Core::FString& Error) const;

        /**
         * @brief 메시 에셋을 캐시에서 조회하거나 파일에서 로드하여 보관합니다.
         * @param[in] Path 불러올 메시 에셋의 경로입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 캐시된 에셋의 비소유 주소이며, 로드에 실패하면 nullptr입니다.
         */
        Asset::UStaticMesh* LoadMesh(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 스켈레탈 메시 파일에서 본·스킨 가중치·기하 데이터를 읽어 캐시에 등록합니다.
         * @param[in] Path 작업에 사용할 파일 또는 프로젝트 상대 에셋 경로입니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return 로드한 메시의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::USkeletalMesh* LoadSkeletalMesh(const std::filesystem::path& Path, Core::FString& Error);
        Asset::USkeleton* LoadSkeleton(const std::filesystem::path& Path, Core::FString& Error);
        Asset::USkeleton* FindLegacySkeleton(const std::filesystem::path& MeshPath) const;

        /**
         * @brief 애니메이션 시퀀스 에셋을 캐시에서 조회하거나 파일에서 로드하여 보관합니다.
         * @param[in] Path 불러올 시퀀스 에셋의 경로입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 캐시된 에셋의 비소유 주소이며, 로드에 실패하면 nullptr입니다.
         */
        Asset::UAnimSequence* LoadAnimSequence(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 텍스처 에셋을 캐시에서 조회하거나 파일에서 로드하여 보관합니다.
         * @param[in] Path 불러올 텍스처 에셋 또는 이미지 파일의 경로입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 캐시된 에셋의 비소유 주소이며, 로드에 실패하면 nullptr입니다.
         */
        Asset::UTexture2D* LoadTexture(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 노멀 맵을 선형 색상으로 로드하고 전용 캐시에 보관합니다.
         * @param[in] Path 불러올 노멀 맵 텍스처의 경로입니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return 캐시가 소유한 텍스처의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::UTexture2D* LoadNormalMap(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 재질 에셋과 텍스처 참조를 로드하여 캐시에 보관합니다.
         * @param[in] Path 프로젝트 머티리얼 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 캐시 소유 머티리얼의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::UMaterial* LoadMaterial(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 시퀀스 에셋을 로드하여 캐시에 보관합니다.
         * @param[in] Path 프로젝트 시퀀스 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 캐시 소유 시퀀스의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::ULevelSequence* LoadSequence(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief Sequences 폴더에 빈 시퀀스 에셋을 만들고 캐시에 보관합니다.
         * @param[in] Name 파일 이름으로 사용할 이름이며 중복되면 번호를 붙입니다.
         * @param[out] Path 생성한 에셋의 Content 상대 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 캐시 소유 시퀀스의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::ULevelSequence* CreateSequence(const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 시퀀스를 에셋 파일로 저장합니다.
         * @param[in] Path 프로젝트 시퀀스 경로입니다.
         * @param[in] Sequence 저장할 시퀀스입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 저장에 성공하면 true입니다.
         */
        bool SaveSequence(const std::filesystem::path& Path, Asset::ULevelSequence& Sequence, Core::FString& Error);

        /**
         * @brief 블렌드 스페이스 에셋을 로드하고 샘플 시퀀스를 해석해 캐시에 보관합니다.
         * @details 로드하지 못한 샘플 시퀀스는 경고만 남기고 비워 둡니다. 블렌드 스페이스는 머티리얼처럼 캐시된 객체를 직접 편집합니다.
         * @param[in] Path 프로젝트 블렌드 스페이스 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 캐시 소유 블렌드 스페이스의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::UBlendSpace* LoadBlendSpace(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 지정한 폴더에 빈 1D 블렌드 스페이스 에셋을 만들고 캐시에 보관합니다.
         * @param[in] Directory Content 기준 폴더입니다.
         * @param[in] Name 파일 이름으로 사용할 이름이며 중복되면 번호를 붙입니다.
         * @param[out] Path 생성한 에셋의 Content 상대 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 캐시 소유 블렌드 스페이스의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::UBlendSpace* CreateBlendSpace1D(const std::filesystem::path& Directory, const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 블렌드 스페이스를 에셋 파일로 저장합니다. 샘플은 시퀀스 경로만 저장합니다.
         * @param[in] Path 프로젝트 블렌드 스페이스 경로입니다.
         * @param[in] BlendSpace 저장할 블렌드 스페이스입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 저장에 성공하면 true입니다.
         */
        bool SaveBlendSpace(const std::filesystem::path& Path, Asset::UBlendSpace& BlendSpace, Core::FString& Error);

        /**
         * @brief 애니메이션 그래프 에셋을 로드해 캐시에 보관합니다. 그래프는 머티리얼처럼 캐시된 객체를 직접 편집합니다.
         * @param[in] Path 프로젝트 그래프 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 캐시 소유 그래프의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::UAnimGraph* LoadAnimGraph(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 지정한 폴더에 빈 애니메이션 그래프 에셋을 만들고 캐시에 보관합니다.
         * @param[in] Directory Content 기준 폴더입니다.
         * @param[in] Name 파일 이름으로 사용할 이름이며 중복되면 번호를 붙입니다.
         * @param[out] Path 생성한 에셋의 Content 상대 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 캐시 소유 그래프의 비소유 주소이며 실패하면 nullptr입니다.
         */
        Asset::UAnimGraph* CreateAnimGraph(const std::filesystem::path& Directory, const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 애니메이션 그래프를 에셋 파일로 저장합니다.
         * @param[in] Path 프로젝트 그래프 경로입니다.
         * @param[in] Graph 저장할 그래프입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 저장에 성공하면 true입니다.
         */
        bool SaveAnimGraph(const std::filesystem::path& Path, Asset::UAnimGraph& Graph, Core::FString& Error);

        /**
         * @brief 캐시 객체의 주소를 유지하면서 파일의 최신 설정을 다시 적용합니다.
         * @param[in] Path 다시 읽을 프로젝트 머티리얼 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 설정과 참조 갱신에 성공하면 true입니다.
         */
        bool ReloadMaterial(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 머티리얼을 안전하게 저장하고 같은 경로의 캐시 객체를 갱신합니다.
         * @param[in] Path 저장할 프로젝트 머티리얼 경로입니다.
         * @param[in] Material 저장할 설정입니다.
         * @param[out] Error 저장 또는 캐시 갱신의 실패 원인입니다.
         * @return 저장과 캐시 갱신에 성공하면 true입니다.
         */
        bool SaveMaterial(const std::filesystem::path& Path, const Asset::UMaterial& Material, Core::FString& Error);

        /**
         * @brief 검증한 CPU 메시 데이터를 바이너리 에셋 파일로 저장합니다.
         * @param[in] Path 저장할 메시 에셋의 경로입니다.
         * @param[in] Mesh 읽거나 직렬화할 메시 데이터입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SaveMesh(const std::filesystem::path& Path, const Asset::UStaticMesh& Mesh, Core::FString& Error);

        /**
         * @brief 스켈레탈 메시의 기하·본 계층·스킨 가중치를 에셋 파일에 저장합니다.
         * @param[in] Path 작업에 사용할 파일 또는 프로젝트 상대 에셋 경로입니다.
         * @param[in] Mesh 조회·저장·포즈 계산에 사용할 메시입니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return 검증과 파일 저장에 성공하면 true입니다.
         */
        bool SaveSkeletalMesh(const std::filesystem::path& Path, const Asset::USkeletalMesh& Mesh, Core::FString& Error);
        bool SaveSkeleton(const std::filesystem::path& Path, const Asset::USkeleton& Skeleton, Core::FString& Error);

        /**
         * @brief 애니메이션 시퀀스의 트랙과 시간 정보를 에셋 파일에 저장하고 같은 경로의 캐시 객체를 갱신합니다.
         * @param[in] Path 저장할 시퀀스 에셋의 경로입니다.
         * @param[in] Sequence 저장할 시퀀스입니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return 검증과 파일 저장에 성공하면 true입니다.
         */
        bool SaveAnimSequence(const std::filesystem::path& Path, const Asset::UAnimSequence& Sequence, Core::FString& Error);

        /**
         * @brief 텍스처의 크기·형식·색 공간과 이미지 데이터를 에셋 파일로 저장합니다.
         * @param[in] Path 저장할 텍스처 에셋의 경로입니다.
         * @param[in] Texture 데이터를 읽거나 뷰를 생성할 텍스처이며 소유권을 이전하지 않습니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SaveTexture(const std::filesystem::path& Path, const Asset::UTexture2D& Texture, Core::FString& Error);

        /**
         * @brief 기본 임포트 옵션으로 외부 에셋을 새 폴더에 가져오고 생성한 에셋 경로를 반환합니다.
         * @param[in] Source 프로젝트에 가져올 외부 파일 경로입니다.
         * @param[in] Directory 새 임포트 폴더를 생성할 Content 상대 디렉터리입니다.
         * @param[out] ImportedPath 성공 시 생성한 주 에셋의 Content 상대 경로를 받습니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ImportAsset(const std::filesystem::path& Source, const std::filesystem::path& Directory, std::filesystem::path& ImportedPath, Core::FString& Error);

        /**
         * @brief 임포트 옵션을 따라 외부 에셋을 새 폴더에 가져오고 생성한 주 에셋 경로를 반환합니다.
         * @param[in] Source 프로젝트에 가져올 외부 파일 경로입니다.
         * @param[in] Directory 새 임포트 폴더를 생성할 Content 상대 디렉터리입니다.
         * @param[in] Option 메시 원본에 적용할 좌표축·메시 구성·재질 임포트 옵션이며 텍스처 원본에는 쓰지 않습니다.
         * @param[out] ImportedPath 성공 시 생성한 주 에셋의 Content 상대 경로를 받습니다. 메시를 여러 개 만들면 첫 메시입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ImportAsset(const std::filesystem::path& Source, const std::filesystem::path& Directory, const Asset::FAssetImportOption& Option, std::filesystem::path& ImportedPath, Core::FString& Error);

        /**
         * @brief 외부 이미지를 읽어 프로젝트의 텍스처 에셋 파일로 저장합니다.
         * @param[in] Source 프로젝트에 가져올 외부 파일 경로입니다.
         * @param[in] Target 프로젝트 콘텐츠 폴더 안에 저장할 대상 에셋 경로입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ImportTexture(const std::filesystem::path& Source, const std::filesystem::path& Target, Core::FString& Error);

        /**
         * @brief 지정한 경로와 그 아래 경로의 캐시 항목을 은퇴시켜 다음 로드가 새 객체를 만들게 합니다.
         * @details 캐시에 올라간 에셋 객체는 내용을 바꾸지 않으므로, 파일을 지우거나 옮긴 뒤 같은 경로를 다시 쓰려면 먼저 은퇴시켜야 합니다.
         * 은퇴한 객체는 매니저가 소멸할 때까지 살아 있어 기존 참조가 끊기지 않습니다.
         * @param[in] Path 은퇴시킬 에셋 파일 또는 폴더 경로입니다.
         */
        void RetireAssets(const std::filesystem::path& Path);

    private:
        /**
         * @brief 원본 메시와 재질·외부 텍스처를 임포트 옵션에 따라 프로젝트 에셋으로 가져와 메시 참조를 저장합니다.
         * @param[in] Source 임포트할 원본 메시 파일 경로입니다.
         * @param[in,out] Target 메시가 하나일 때 기록할 프로젝트 .masset 경로이며, 성공하면 첫 메시를 저장한 경로로 바뀝니다.
         * 메시가 여러 개이면 같은 폴더에 메시 이름으로 저장합니다.
         * @param[in] Option 좌표축·메시 구성·재질 임포트 옵션입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 메시와 연결 에셋을 저장하면 true이며 가져오기·검증·저장 실패 시 false입니다.
         */
        bool ImportMesh(const std::filesystem::path& Source, std::filesystem::path& Target, const Asset::FAssetImportOption& Option, Core::FString& Error);

        /**
         * @brief 캐시 객체의 주소를 유지하면서 머티리얼 설정과 텍스처 참조를 갱신합니다.
         * @param[in,out] Target 설정을 적용할 기존 객체입니다.
         * @param[in] Source 복사할 설정을 제공하는 객체입니다.
         * @param[out] Error 텍스처 참조를 해석할 수 없는 경우의 원인입니다.
         * @return 텍스처 참조까지 복원하면 true입니다.
         */
        bool ApplyMaterialSettings(Asset::UMaterial& Target, const Asset::UMaterial& Source, Core::FString& Error);

    private:
        /**
         * @brief 에셋 접근을 제한하는 프로젝트 콘텐츠 폴더의 절대 경로입니다.
         */
        std::filesystem::path mContentRoot{};

        /**
         * @brief 정규화한 상대 경로별로 소유하는 메시 에셋 캐시입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::UStaticMesh>> mMeshes{};

        /**
         * @brief 프로젝트 상대 경로별로 소유하는 스켈레탈 메시 캐시입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::USkeletalMesh>> mSkeletalMeshes{};
        Core::TMap<Core::FString, std::unique_ptr<Asset::USkeleton>> mSkeletons{};
        Core::TMap<Core::FString, std::unique_ptr<Asset::USkeleton>> mLegacySkeletons{};

        /**
         * @brief 프로젝트 상대 경로별로 소유하는 애니메이션 시퀀스 캐시입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::UAnimSequence>> mAnimSequences{};

        /**
         * @brief 정규화한 상대 경로별로 소유하는 텍스처 에셋 캐시입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::UTexture2D>> mTextures{};

        /**
         * @brief 정규화한 상대 경로별로 소유하는 머티리얼 캐시입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::UTexture2D>> mNormalMaps{};

        /**
         * @brief 경로별로 캐시한 머티리얼의 소유 저장소입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::UMaterial>> mMaterials{};

        /**
         * @brief 경로별로 캐시한 시퀀스의 소유 저장소입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::ULevelSequence>> mSequences{};

        /**
         * @brief 경로별로 캐시한 블렌드 스페이스의 소유 저장소입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::UBlendSpace>> mBlendSpaces{};

        /**
         * @brief 경로별로 캐시한 애니메이션 그래프의 소유 저장소입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Asset::UAnimGraph>> mAnimGraphs{};

        /**
         * @brief 경로 캐시에서 빠졌지만 기존 참조를 위해 매니저 수명 동안 유지하는 에셋 객체입니다.
         */
        Core::TArray<std::unique_ptr<CoreUObject::UObject>> mRetired{};
    };

}
