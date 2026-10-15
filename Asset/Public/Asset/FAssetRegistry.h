/**
 * @file FAssetRegistry.h
 * @brief 공용 에셋 저장소의 전역 접근, 로딩과 식별자 해석을 정의합니다.
 */
#pragma once
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/IAssetRegistryMutator.h"
#include "Asset/FAssetEntry.h"
#include "Asset/UMaterial.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

/** @brief 전역 에셋 레지스트리를 활성화하고 소유하는 엔진입니다. */
class UEngine;

/** @brief 에셋 검색, 로딩과 핸들 해석을 제공하며 활성 엔진의 공용 에셋 접근점을 공개합니다. @details 별도로 생성한 레지스트리는 전역 접근점을 변경하지 않습니다. 전역 레지스트리의 소유와 해제는 활성 엔진이 담당합니다. */
class FAssetRegistry : public IAssetRegistry, public IAssetRegistryMutator {
public:
    /** @brief 0부터 1까지의 로딩 진행률과 상태 문자열을 받는 콜백입니다. */
    using FProgressCallback = std::function<void(float, const std::string&)>;

public:
    /** @brief 전역으로 등록하지 않은 빈 레지스트리를 생성합니다. */
    FAssetRegistry() = default;
    /** @brief 이 객체가 활성 레지스트리이면 전역 접근점을 비운 뒤 에셋을 해제합니다. */
    ~FAssetRegistry() override;

    /** @brief 에셋 소유권의 복사 생성을 금지합니다. */
    FAssetRegistry(const FAssetRegistry&) = delete;
    /** @brief 에셋 소유권의 복사 대입을 금지합니다. */
    FAssetRegistry& operator=(const FAssetRegistry&) = delete;

    /** @brief 레지스트리 주소와 핸들의 안정성을 위해 이동 생성을 금지합니다. */
    FAssetRegistry(FAssetRegistry&&) = delete;
    /** @brief 레지스트리 주소와 핸들의 안정성을 위해 이동 대입을 금지합니다. */
    FAssetRegistry& operator=(FAssetRegistry&&) = delete;

public:
    /**
     * @brief 활성 엔진이 소유한 공용 에셋 레지스트리를 반환합니다.
     * @return 활성 레지스트리의 비소유 참조입니다.
     * @pre 엔진 초기화가 완료되어 있어야 합니다. 활성 레지스트리가 없으면 Critical 오류를 보고합니다.
     * @note 레지스트리 접근 가능 여부는 에셋 로딩 완료를 의미하지 않습니다. 엔진 종료 이후에는 참조를 사용하지 않습니다.
     */
    static FAssetRegistry& Get();
    /**
     * @brief 활성 에셋 레지스트리를 생성하지 않고 조회합니다.
     * @return 엔진 소유 레지스트리의 비소유 포인터입니다. 초기화 전과 레지스트리 해제 후에는 nullptr입니다.
     * @note 초기화와 종료 및 에셋 수정은 외부에서 동기화해야 합니다.
     */
    static FAssetRegistry* TryGet() noexcept;

    /**
     * @brief 활성 레지스트리의 생성 또는 전체 초기화를 구분하는 세대 번호를 반환합니다.
     * @return 활성 서비스가 없으면 0이며, 서비스 활성화와 활성 레지스트리의 Reset마다 새로운 값입니다.
     * @details 에셋 핸들, GUID와 수정 번호가 재사용되어도 GPU 및 프리뷰 캐시의 이전 상태를 구분할 수 있습니다.
     * @note 초기화와 종료 및 Reset은 외부에서 동기화해야 합니다.
     */
    static Uint64 GetGeneration() noexcept;

    /** @brief Content 경로의 에셋을 검색하고 타입별로 로드합니다. @param ProgressCallback 선택적인 로딩 진행 콜백입니다. @return 필요한 로드와 시스템 에셋 검증이 성공하면 true입니다. @note 전역 접근점의 등록은 이 함수가 아닌 엔진 초기화에서 수행합니다. */
    bool Initialize(const FProgressCallback& ProgressCallback = {});

    /** @brief 지정 디렉터리의 에셋과 메타데이터를 검색합니다. @param Directory 검색할 실제 디렉터리입니다. @return 검색에 성공하면 true입니다. */
    bool DiscoverAssets(const std::filesystem::path& Directory);
    /** @brief 검색된 에셋 중 지정 타입을 로드합니다. @param AssetType 로드할 에셋 타입입니다. @return 대상 에셋 로드에 성공하면 true입니다. */
    bool LoadAssetsOfType(EAssetType AssetType);

    /** @brief 검색에 사용하는 실제 Content 루트 경로를 반환합니다. */
    const std::filesystem::path& GetContentRoot() const;

    /** @brief 검색된 에셋 항목 목록을 읽기 전용으로 반환합니다. */
    const TArray<FAssetEntry>& GetAssetEntries() const;

    /** @brief 가상 경로로 에셋을 찾습니다. @param AssetPath 검색할 가상 경로입니다. @return 에셋이 없으면 빈 핸들입니다. */
    FAssetHandle FindAsset(const FAssetPath& AssetPath) const override;
    /** @brief 영속 식별자로 에셋을 찾습니다. @param PersistentGuid 검색할 GUID입니다. @return 에셋이 없으면 빈 핸들입니다. */
    FAssetHandle FindAsset(const FGuid& PersistentGuid) const override;
    /** @brief 에셋의 가상 경로를 조회합니다. @param Handle 조회할 핸들입니다. @return 등록된 경로의 비소유 포인터이며 없으면 nullptr입니다. */
    const FAssetPath* GetAssetPath(FAssetHandle Handle) const override;
    /** @brief 에셋의 영속 식별자를 조회합니다. @param Handle 조회할 핸들입니다. @return 등록된 GUID의 비소유 포인터이며 없으면 nullptr입니다. */
    const FGuid* GetAssetGuid(FAssetHandle Handle) const override;

    /** @brief 지정 클래스와 호환되는 로드된 에셋의 핸들을 반환합니다. @param AssetType 조회할 리플렉션 클래스입니다. */
    TArray<FAssetHandle> GetAssetHandles(const UClass& AssetType) const override;

    /** @brief 파이프라인의 렌더 모드를 갱신합니다. @param Handle 파이프라인 핸들입니다. @param Mode 적용할 렌더 모드입니다. */
    void SetPipelineRenderMode(FAssetHandle Handle, ERenderMode Mode) override;
    /** @brief 폰트 글리프를 조회하거나 생성합니다. @param Handle 폰트 핸들입니다. @param CodePoint 글자의 유니코드 값입니다. @return 글리프의 비소유 포인터이며 실패하면 nullptr입니다. */
    const FFontGlyph* GetOrCreateFontGlyph(FAssetHandle Handle, char32_t CodePoint) override;

    /** @brief 에셋과 식별자 매핑을 제거합니다. @param Handle 제거할 핸들입니다. @return 제거에 성공하면 true입니다. */
    bool RemoveAsset(FAssetHandle Handle);

    /** @brief OBJ 파일을 지정 가상 폴더의 메시 에셋으로 가져옵니다. @param SourceObjPath 원본 OBJ 경로입니다. @param TargetVirtualFolder 대상 가상 폴더입니다. @return 실패하면 빈 핸들입니다. */
    FAssetHandle ImportMesh(const std::filesystem::path& SourceObjPath, const FString& TargetVirtualFolder);
    /** @brief 외부 파일을 뷰어용 에셋으로 로드합니다. @param SourcePath 원본 파일 경로입니다. @return 실패하면 빈 핸들입니다. */
    FAssetHandle LoadViewerAsset(const std::filesystem::path& SourcePath);

    /** @brief 핸들을 수정 가능한 타입 지정 에셋으로 해석합니다. @tparam T 요구하는 UAsset 파생 타입입니다. @param Handle 조회할 핸들입니다. @return 타입이 맞는 비소유 포인터이며 실패하면 nullptr입니다. */
    template <typename T>
        requires std::is_base_of_v<UAsset, T>
    T* ResolveAsset(FAssetHandle Handle);

    /** @brief 핸들을 읽기 전용 타입 지정 에셋으로 해석합니다. @tparam T 요구하는 UAsset 파생 타입입니다. @param Handle 조회할 핸들입니다. @return 타입이 맞는 비소유 포인터이며 실패하면 nullptr입니다. */
    template <typename T>
        requires std::is_base_of_v<UAsset, T>
    const T* ResolveAsset(FAssetHandle Handle) const;

    /** @brief 유효한 에셋에 수정 함수를 적용합니다. @tparam T 요구하는 에셋 타입입니다. @tparam Func 수정 함수 타입입니다. @param Handle 수정할 핸들입니다. @param Modifier 에셋 참조를 받는 함수입니다. */
    template <typename T, typename Func>
        requires std::is_base_of_v<UAsset, T>
    void ModifyAsset(FAssetHandle Handle, Func&& Modifier);

    /** @brief 에셋 목록과 식별자 매핑을 비웁니다. 전역 연결은 유지하며 활성 레지스트리이면 세대를 갱신해 기존 캐시를 무효화합니다. */
    void Reset();
    /** @brief 로드된 머티리얼의 에셋 참조를 연결합니다. */
    void Finalize();

    /** @brief 기본 정적 메시 머티리얼 핸들을 반환하며 사용할 수 없으면 빈 핸들입니다. */
    FAssetHandle EnsureDefaultStaticMeshMaterial() const override;
    /** @brief 기본 정적 메시 파이프라인 핸들을 반환하며 사용할 수 없으면 빈 핸들입니다. */
    FAssetHandle EnsureDefaultStaticMeshPipeline() const override;

private:
    /** @brief 전역 레지스트리 활성화 권한을 소유 엔진에만 부여합니다. */
    friend class UEngine;

    /** @brief 이 레지스트리를 활성 엔진의 전역 에셋 접근점으로 등록합니다. 다른 레지스트리가 활성 상태이면 Critical 오류를 보고합니다. */
    void ActivateGlobal();
    /** @brief 핸들의 에셋 객체를 해석합니다. @param Handle 조회할 핸들입니다. @return 객체가 없으면 nullptr입니다. */
    const UObject* ResolveAssetObject(FAssetHandle Handle) const override;

    /** @brief 가상 Content 폴더를 실제 경로로 변환합니다. @param VirtualFolder 변환할 가상 경로입니다. */
    std::filesystem::path ResolveContentFolder(const FString& VirtualFolder) const;
    /** @brief 진행률을 갱신하며 지정 타입의 에셋을 로드합니다. @param AssetType 대상 타입입니다. @param LoadedAssetCount 누적 처리 개수입니다. @param TotalAssetCount 전체 대상 개수입니다. @param ProgressCallback 진행 콜백입니다. @return 대상 로드가 성공하면 true입니다. */
    bool LoadAssetsOfType(EAssetType AssetType, std::size_t& LoadedAssetCount, std::size_t TotalAssetCount, const FProgressCallback& ProgressCallback);
    /** @brief 기본 머티리얼과 파이프라인 등 시스템 필수 에셋을 검증합니다. */
    bool EnsureSystemAssets();
    /** @brief 에셋 파일 하나의 정보를 검색합니다. @param FilePath 검색할 실제 파일 경로입니다. */
    bool DiscoverAssetFile(const std::filesystem::path& FilePath);
    /** @brief 텍스처 항목을 로드합니다. @param Entry 갱신할 에셋 항목입니다. */
    bool LoadTexture(FAssetEntry& Entry);
    /** @brief 폰트 항목을 로드합니다. @param Entry 갱신할 에셋 항목입니다. */
    bool LoadFont(FAssetEntry& Entry);
    /** @brief 파이프라인 항목을 로드합니다. @param Entry 갱신할 에셋 항목입니다. */
    bool LoadPipeline(FAssetEntry& Entry);
    /** @brief 머티리얼 항목을 로드합니다. @param Entry 갱신할 에셋 항목입니다. */
    bool LoadMaterial(FAssetEntry& Entry);
    /** @brief 메시 항목을 로드합니다. @param Entry 갱신할 에셋 항목입니다. */
    bool LoadMesh(FAssetEntry& Entry);
    /** @brief 검색한 에셋의 경로와 식별자를 등록합니다. @param AssetPath 가상 경로입니다. @param PhysicalPath 실제 파일 경로입니다. @param SidecarPath 메타데이터 경로입니다. @param PersistentGuid 영속 식별자입니다. @param AssetType 에셋 타입입니다. @param Entry 등록 결과 항목입니다. @return 등록에 성공하면 true입니다. */
    bool RegisterDiscoveredAsset(const FAssetPath& AssetPath, const std::filesystem::path& PhysicalPath, const std::filesystem::path& SidecarPath, const FGuid& PersistentGuid, EAssetType AssetType, FAssetEntry& Entry);

    /** @brief 실제 파일 경로를 가상 에셋 경로로 변환합니다. @param PhysicalPath 변환할 실제 경로입니다. */
    FAssetPath MakeAssetPath(const std::filesystem::path& PhysicalPath) const;
    /** @brief 파일 경로에서 에셋 타입을 판별합니다. @param FilePath 검사할 파일 경로입니다. */
    static EAssetType GetAssetType(const std::filesystem::path& FilePath);
    /** @brief 에셋의 메타데이터 파일 경로를 만듭니다. @param AssetPath 실제 에셋 파일 경로입니다. */
    static std::filesystem::path MakeSidecarPath(const std::filesystem::path& AssetPath);
    /** @brief 메타데이터를 읽거나 새로 생성합니다. @param SidecarPath 메타데이터 파일 경로입니다. @param AssetType 에셋 타입입니다. @param Entry 갱신할 항목입니다. @return 처리에 성공하면 true입니다. */
    static bool LoadOrCreateMetadata(const std::filesystem::path& SidecarPath, EAssetType AssetType, FAssetEntry& Entry);
    /** @brief 파이프라인 계열 구성 파일인지 검사합니다. @param FilePath 검사할 실제 경로입니다. */
    static bool IsPipelineFamilyUnit(const std::filesystem::path& FilePath);
    /** @brief 파이프라인 계열 폴더의 첫 구성 파일을 찾습니다. @param FamilyDirectory 검색할 실제 폴더입니다. */
    static std::filesystem::path FindFirstPipelineFamilyUnit(const std::filesystem::path& FamilyDirectory);

    /** @brief 사용 가능한 에셋 핸들을 할당합니다. */
    FAssetHandle AllocateHandle();
    /** @brief 수정 가능한 에셋 항목을 찾습니다. @param Handle 조회할 핸들입니다. @return 없으면 nullptr입니다. */
    FAssetEntry* FindEntry(FAssetHandle Handle);
    /** @brief 읽기 전용 에셋 항목을 찾습니다. @param Handle 조회할 핸들입니다. @return 없으면 nullptr입니다. */
    const FAssetEntry* FindEntry(FAssetHandle Handle) const;
    /** @brief 핸들의 경로와 GUID 매핑을 제거합니다. @param Handle 제거할 핸들입니다. */
    void RemoveHandleMappings(FAssetHandle Handle);

private:
    /** @brief 에셋 정보와 로드한 객체의 소유 목록입니다. */
    TArray<FAssetEntry> mAssets{};
    /** @brief 재사용 가능한 에셋 핸들 목록입니다. */
    TArray<FAssetHandle> mFreeHandles{};

    /** @brief 가상 경로에서 핸들을 찾는 매핑입니다. */
    TMap<FAssetPath, FAssetHandle> mPathToHandle{};
    /** @brief 영속 식별자에서 핸들을 찾는 매핑입니다. */
    TMap<FGuid, FAssetHandle> mGuidToHandle{};

    /** @brief 에셋 검색에 사용하는 실제 Content 루트 경로입니다. */
    std::filesystem::path mContentRoot{};
};

template <typename T>
    requires std::is_base_of_v<UAsset, T>
T* FAssetRegistry::ResolveAsset(FAssetHandle Handle) {
    FAssetEntry* Entry{FindEntry(Handle)};

    if (Entry == nullptr || Entry->mAsset == nullptr || !Entry->mAsset->GetClass()->IsChildOf(T::StaticClass())) {
        return nullptr;
    }

    return static_cast<T*>(Entry->mAsset.get());
}

template <typename T>
    requires std::is_base_of_v<UAsset, T>
const T* FAssetRegistry::ResolveAsset(FAssetHandle Handle) const {
    const FAssetEntry* Entry{FindEntry(Handle)};

    if (Entry == nullptr || Entry->mAsset == nullptr || !Entry->mAsset->GetClass()->IsChildOf(T::StaticClass())) {
        return nullptr;
    }

    return static_cast<const T*>(Entry->mAsset.get());
}

template <typename T, typename Func>
    requires std::is_base_of_v<UAsset, T>
void FAssetRegistry::ModifyAsset(FAssetHandle Handle, Func&& Modifier) {
    T* Asset{ResolveAsset<T>(Handle)};

    if (Asset == nullptr) {
        return;
    }

    std::invoke(std::forward<Func>(Modifier), *Asset);
}
