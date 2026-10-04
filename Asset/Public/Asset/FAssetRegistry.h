#pragma once
#include "CoreUObject/UObject.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/IAssetRegistryMutator.h"
#include "Asset/FAssetEntry.h"
#include "Asset/UMaterial.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>

class FAssetRegistry : public IAssetRegistry, public IAssetRegistryMutator {
public:
    using FProgressCallback = std::function<void(float, const std::string&)>;

public:
    FAssetRegistry() = default;
    ~FAssetRegistry() override = default;

    FAssetRegistry(const FAssetRegistry&) = delete;
    FAssetRegistry& operator=(const FAssetRegistry&) = delete;

    FAssetRegistry(FAssetRegistry&&) = delete;
    FAssetRegistry& operator=(FAssetRegistry&&) = delete;

public:
    bool Initialize(const FProgressCallback& ProgressCallback = {});

    bool DiscoverAssets(const std::filesystem::path& Directory);
    bool LoadAssetsOfType(EAssetType AssetType);

    const std::filesystem::path& GetContentRoot() const;

    const TArray<FAssetEntry>& GetAssetEntries() const;

    FAssetHandle FindAsset(const FAssetPath& AssetPath) const override;
    FAssetHandle FindAsset(const FGuid& PersistentGuid) const override;
    const FAssetPath* GetAssetPath(FAssetHandle Handle) const override;
    const FGuid* GetAssetGuid(FAssetHandle Handle) const override;

    TArray<FAssetHandle> GetAssetHandles(const FTypeInfo& AssetType) const override;

    void SetPipelineRenderMode(FAssetHandle Handle, ERenderMode Mode) override;
    const FFontGlyph* GetOrCreateFontGlyph(FAssetHandle Handle, char32_t CodePoint) override;

    bool RemoveAsset(FAssetHandle Handle);

    FAssetHandle ImportMesh(const std::filesystem::path& SourceObjPath, const FString& TargetVirtualFolder);
    FAssetHandle LoadViewerAsset(const std::filesystem::path& SourcePath);

    template <typename T>
        requires std::is_base_of_v<UAsset, T>
    T* ResolveAsset(FAssetHandle Handle);

    template <typename T>
        requires std::is_base_of_v<UAsset, T>
    const T* ResolveAsset(FAssetHandle Handle) const;

    template <typename T, typename Func>
        requires std::is_base_of_v<UAsset, T>
    void ModifyAsset(FAssetHandle Handle, Func&& Modifier);

    auto GetAssetList() const;

    void Reset();
    void Finalize();

    FAssetHandle EnsureDefaultStaticMeshMaterial() const override;
    FAssetHandle EnsureDefaultStaticMeshPipeline() const override;

private:
    const UObject* ResolveAssetObject(FAssetHandle Handle) const override;

    std::filesystem::path ResolveContentFolder(const FString& VirtualFolder) const;
    bool LoadAssetsOfType(EAssetType AssetType, std::size_t& LoadedAssetCount, std::size_t TotalAssetCount, const FProgressCallback& ProgressCallback);
    bool EnsureSystemAssets();
    bool DiscoverAssetFile(const std::filesystem::path& FilePath);
    bool LoadTexture(FAssetEntry& Entry);
    bool LoadFont(FAssetEntry& Entry);
    bool LoadPipeline(FAssetEntry& Entry);
    bool LoadMaterial(FAssetEntry& Entry);
    bool LoadMesh(FAssetEntry& Entry);
    bool RegisterDiscoveredAsset(const FAssetPath& AssetPath, const std::filesystem::path& PhysicalPath, const std::filesystem::path& SidecarPath, const FGuid& PersistentGuid, EAssetType AssetType, FAssetEntry& Entry);

    FAssetPath MakeAssetPath(const std::filesystem::path& PhysicalPath) const;
    static EAssetType GetAssetType(const std::filesystem::path& FilePath);
    static std::filesystem::path MakeSidecarPath(const std::filesystem::path& AssetPath);
    static bool LoadOrCreateMetadata(const std::filesystem::path& SidecarPath, EAssetType AssetType, FAssetEntry& Entry);
    static bool IsPipelineFamilyUnit(const std::filesystem::path& FilePath);
    static std::filesystem::path FindFirstPipelineFamilyUnit(const std::filesystem::path& FamilyDirectory);

    FAssetHandle AllocateHandle();
    FAssetEntry* FindEntry(FAssetHandle Handle);
    const FAssetEntry* FindEntry(FAssetHandle Handle) const;
    void RemoveHandleMappings(FAssetHandle Handle);

private:
    TArray<FAssetEntry> mAssets{};
    TArray<FAssetHandle> mFreeHandles{};

    TMap<FAssetPath, FAssetHandle> mPathToHandle{};
    TMap<FGuid, FAssetHandle> mGuidToHandle{};

    std::filesystem::path mContentRoot{};
};

template <typename T>
    requires std::is_base_of_v<UAsset, T>
T* FAssetRegistry::ResolveAsset(FAssetHandle Handle) {
    FAssetEntry* Entry{FindEntry(Handle)};

    if (Entry == nullptr || Entry->mAsset == nullptr || !Entry->mAsset->GetTypeInfo()->IsA(T::StaticTypeInfo())) {
        return nullptr;
    }

    return static_cast<T*>(Entry->mAsset.get());
}

template <typename T>
    requires std::is_base_of_v<UAsset, T>
const T* FAssetRegistry::ResolveAsset(FAssetHandle Handle) const {
    const FAssetEntry* Entry{FindEntry(Handle)};

    if (Entry == nullptr || Entry->mAsset == nullptr || !Entry->mAsset->GetTypeInfo()->IsA(T::StaticTypeInfo())) {
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
