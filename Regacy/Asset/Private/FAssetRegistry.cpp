#include "pch.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Serialization/FJsonFile.h"
#include "Asset/FAssetRegistry.h"
#include "Core/Base/ErrorHandler.h"
#include "Asset/UFreeTypeFont.h"
#include "Asset/FLODSettings.h"
#include "Asset/UMesh.h"
#include "Asset/USurfaceOpaque.h"
#include "Asset/UTexture.h"
#include "Asset/FObjImporter.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Core/Console/Console.h"
#include "Asset/FMeshAssetSerializer.h"
#include "Asset/FMaterialAssetSerializer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace {
    FAssetRegistry* AssetRegistryInstance{};
    Uint64 AssetRegistryGeneration{};
    constexpr const char* DefaultStaticMeshMaterialAssetPath{"/Game/System/Material/Default.mtl.materials/M_Default.material"};
    constexpr const char* DefaultCheckerboardTexturePath{"/Game/Texture/checkerboard.png"};
    constexpr const char* GizmoPipelineAssetPath{"/Game/Pipeline/Gizmo.json"};

    std::filesystem::path MakeMaterialPath(const std::filesystem::path& Folder, FName MaterialName) {
        std::string Name{"M_"};

        for (unsigned char Character : MaterialName.ToString()) {
            if ((Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || (Character >= '0' && Character <= '9') || Character == '-') {
                Name += static_cast<char>(Character);
            } else {
                constexpr char Digits[]{"0123456789ABCDEF"};

                Name += '_';
                Name += Digits[Character >> 4];
                Name += Digits[Character & 15];
            }
        }

        return Folder / (Name + ".material");
    }

    FString GetLowercaseExtension(const std::filesystem::path& FilePath) {
        FString Extension{FilePath.extension().generic_string().c_str()};
        std::ranges::transform(Extension, Extension.begin(), [](unsigned char Character) {
            return static_cast<char>(std::tolower(Character));
        });

        return Extension;
    }
}

FAssetRegistry::~FAssetRegistry() {
    if (AssetRegistryInstance == this) {
        AssetRegistryInstance = nullptr;
    }
}

FAssetRegistry& FAssetRegistry::Get() {
    ErrorHandler::Report(AssetRegistryInstance == nullptr, "FAssetRegistry", "No engine asset registry is active", ErrorHandler::EErrorLevel::Critical);

    return *AssetRegistryInstance;
}

FAssetRegistry* FAssetRegistry::TryGet() noexcept {
    return AssetRegistryInstance;
}

Uint64 FAssetRegistry::GetGeneration() noexcept {
    return AssetRegistryInstance != nullptr ? AssetRegistryGeneration : 0;
}

void FAssetRegistry::ActivateGlobal() {
    ErrorHandler::Report(AssetRegistryInstance != nullptr && AssetRegistryInstance != this, "FAssetRegistry", "Another asset registry is already active", ErrorHandler::EErrorLevel::Critical);

    if (AssetRegistryInstance != this) {
        ++AssetRegistryGeneration;
        AssetRegistryInstance = this;
    }
}

bool FAssetRegistry::Initialize(const FProgressCallback& ProgressCallback) {
    if (ProgressCallback) {
        ProgressCallback(0.0f, "Discovering assets");
    }

    if (!DiscoverAssets(std::filesystem::current_path() / "Content")) {
        return false;
    }

    const std::size_t TotalAssetCount{static_cast<std::size_t>(std::ranges::count_if(mAssets, [](const FAssetEntry& Entry) {
        return Entry.mAssetType != EAssetType::END && Entry.mAsset == nullptr;
    }))};

    std::size_t LoadedAssetCount{};
    const std::array AssetTypes{EAssetType::Texture, EAssetType::Font, EAssetType::Pipeline, EAssetType::Material, EAssetType::Mesh};
    bool LoadedAllAssets{true};

    for (const EAssetType AssetType : AssetTypes) {
        LoadedAllAssets = LoadAssetsOfType(AssetType, LoadedAssetCount, TotalAssetCount, ProgressCallback) && LoadedAllAssets;
    }

    if (ProgressCallback) {
        ProgressCallback(1.0f, "Validating system assets");
    }

    return EnsureSystemAssets() && LoadedAllAssets;
}

bool FAssetRegistry::DiscoverAssets(const std::filesystem::path& Directory) {
    std::error_code ErrorCode{};
    const std::filesystem::path AbsoluteDirectory{std::filesystem::absolute(Directory, ErrorCode).lexically_normal()};

    if (ErrorCode || !std::filesystem::is_directory(AbsoluteDirectory, ErrorCode)) {
        return false;
    }

    mContentRoot = AbsoluteDirectory;

    std::filesystem::recursive_directory_iterator Iterator{mContentRoot, std::filesystem::directory_options::skip_permission_denied, ErrorCode};
    const std::filesystem::recursive_directory_iterator End{};

    bool BDiscoveredAll{true};

    while (!ErrorCode && Iterator != End) {
        const std::filesystem::directory_entry Entry{*Iterator};

        if (Entry.is_regular_file(ErrorCode)) {
            BDiscoveredAll = DiscoverAssetFile(Entry.path()) && BDiscoveredAll;
        }

        Iterator.increment(ErrorCode);

        if (ErrorCode) {
            ErrorCode.clear();
        }
    }

    return BDiscoveredAll;
}

bool FAssetRegistry::LoadAssetsOfType(EAssetType AssetType) {
    std::size_t LoadedAssetCount{};

    return LoadAssetsOfType(AssetType, LoadedAssetCount, 0, {});
}

bool FAssetRegistry::LoadAssetsOfType(EAssetType AssetType, std::size_t& LoadedAssetCount, std::size_t TotalAssetCount, const FProgressCallback& ProgressCallback) {
    bool BLoadedAll{true};

    for (FAssetEntry& Entry : mAssets) {
        if (Entry.mAssetType != AssetType || Entry.mAsset != nullptr) {
            continue;
        }

        std::string Status{"Loading "};

        Status += Entry.mAssetPath.mPath.c_str();

        if (ProgressCallback) {
            const float Progress{TotalAssetCount == 0 ? 0.0f : static_cast<float>(LoadedAssetCount) / static_cast<float>(TotalAssetCount)};

            ProgressCallback(Progress, Status);
        }

        bool BLoaded{false};

        if (AssetType == EAssetType::Texture) {
            BLoaded = LoadTexture(Entry);
        } else if (AssetType == EAssetType::Font) {
            BLoaded = LoadFont(Entry);
        } else if (AssetType == EAssetType::Pipeline) {
            BLoaded = LoadPipeline(Entry);
        } else if (AssetType == EAssetType::Material) {
            BLoaded = LoadMaterial(Entry);
        } else if (AssetType == EAssetType::Mesh) {
            BLoaded = LoadMesh(Entry);
        }

        BLoadedAll = BLoadedAll && BLoaded;
        ++LoadedAssetCount;

        if (ProgressCallback) {
            const float Progress{TotalAssetCount == 0 ? 1.0f : static_cast<float>(LoadedAssetCount) / static_cast<float>(TotalAssetCount)};

            ProgressCallback(Progress, Status);
        }
    }

    return BLoadedAll;
}

FAssetHandle FAssetRegistry::FindAsset(const FAssetPath& AssetPath) const {
    const auto It{mPathToHandle.find(AssetPath)};

    if (It == mPathToHandle.end()) {
        return {};
    }

    return It->second;
}

FAssetHandle FAssetRegistry::FindAsset(const FGuid& PersistentGuid) const {
    const auto It{mGuidToHandle.find(PersistentGuid)};

    return It != mGuidToHandle.end() ? It->second : FAssetHandle{};
}

const FAssetPath* FAssetRegistry::GetAssetPath(FAssetHandle Handle) const {
    const FAssetEntry* Entry{FindEntry(Handle)};

    return Entry != nullptr && Entry->mAssetPath ? &Entry->mAssetPath : nullptr;
}

const FGuid* FAssetRegistry::GetAssetGuid(FAssetHandle Handle) const {
    const FAssetEntry* Entry{FindEntry(Handle)};

    return Entry != nullptr && Entry->mPersistentGuid.IsValid() ? &Entry->mPersistentGuid : nullptr;
}

bool FAssetRegistry::RemoveAsset(FAssetHandle Handle) {
    FAssetEntry* Entry{FindEntry(Handle)};

    if (Entry == nullptr) {
        return false;
    }

    RemoveHandleMappings(Handle);
    Entry->mAsset.reset();
    Entry->mAssetPath = {};
    Entry->mPhysicalPath.clear();
    Entry->mSidecarPath.clear();
    Entry->mPersistentGuid = {};
    Entry->mAssetType = EAssetType::END;
    Entry->mHandle = FAssetHandle{Handle.mId, Handle.mGeneration + 1};
    mFreeHandles.push_back(Entry->mHandle);

    return true;
}

FAssetHandle FAssetRegistry::ImportMesh(const std::filesystem::path& SourceObjPath, const FString& TargetVirtualFolder) {
    std::error_code Error{};
    const std::filesystem::path Source{std::filesystem::absolute(SourceObjPath, Error).lexically_normal()};
    const std::filesystem::path Folder{ResolveContentFolder(TargetVirtualFolder)};

    if (Error || Folder.empty() || GetLowercaseExtension(Source) != ".obj" || !std::filesystem::is_regular_file(Source, Error)) {
        return {};
    }

    std::filesystem::path Target{Folder / Source.filename()};

    Target.replace_extension(".bin");

    if (std::filesystem::exists(Target, Error) || std::filesystem::exists(MakeSidecarPath(Target), Error) || Error) {
        return {};
    }

    const std::filesystem::path MaterialFolder{Target.parent_path() / (Target.stem().string() + ".materials")};

    if (std::filesystem::exists(MaterialFolder, Error) || Error) {
        return {};
    }

    return ImportMeshAsset(Source, Target, false);
}

FAssetHandle FAssetRegistry::LoadViewerAsset(const std::filesystem::path& SourcePath) {
    std::error_code Error{};
    const std::filesystem::path Source{std::filesystem::absolute(SourcePath, Error).lexically_normal()};

    if (Error || !std::filesystem::is_regular_file(Source, Error)) {
        return {};
    }

    for (const FAssetEntry& Entry : mAssets) {
        if (Entry.mAsset != nullptr && Entry.mPhysicalPath == Source) {
            return Entry.mHandle;
        }
    }

    const FString Extension{GetLowercaseExtension(Source)};

    if (Extension == ".obj" || Extension == ".bin") {
        return ImportMeshAsset(Source, {}, false);
    }

    if (Extension == ".mtl") {
        TArray<FAssetHandle> Existing{};

        for (const FAssetEntry& Entry : mAssets) {
            if (Entry.mAssetType != EAssetType::END) {
                Existing.push_back(Entry.mHandle);
            }
        }

        const TArray<FMeshMaterialSlot> Materials{ImportMaterialLibrary(Source, {})};

        if (Materials.empty()) {
            for (FAssetHandle Handle : GetAssetHandles(*UAsset::StaticClass())) {
                if (std::ranges::find(Existing, Handle) == Existing.end()) {
                    RemoveAsset(Handle);
                }
            }
        }

        return Materials.empty() ? FAssetHandle{} : Materials.front().mMaterial.GetHandle();
    }

    if (Extension == ".material") {
        std::unique_ptr<USurfaceOpaque> Material{NewObject<USurfaceOpaque>()};

        if (!FMaterialAssetSerializer::Load(Source, *Material)) {
            return {};
        }

        Material->Finalize(this);

        return RegisterImportedAsset(Source, {}, std::move(Material));
    }

    return ImportTexture(Source, {});
}

bool FAssetRegistry::SaveMaterial(FAssetHandle Handle) {
    const FAssetEntry* Entry{FindEntry(Handle)};
    const USurfaceOpaque* Material{ResolveAsset<USurfaceOpaque>(Handle)};

    return Entry != nullptr && Material != nullptr && Entry->mPhysicalPath.extension() == ".material" && FMaterialAssetSerializer::Save(Entry->mPhysicalPath, *Material);
}

FAssetReference FAssetRegistry::ImportLegacyMaterial(const FAssetReference& Library, FName Slot, const FAssetReference& Pipeline) {
    FAssetReference Material{Library};

    Material.Resolve(this);

    if (Library.GetPath().mPath.ends_with(".mtl")) {
        const FAssetPath SlotPath{MakeMaterialPath(std::filesystem::path{Library.GetPath().mPath + ".materials"}, Slot).generic_string().c_str()};
        const FAssetHandle SlotMaterial{FindAsset(SlotPath)};

        if (SlotMaterial) {
            Material = MakeReference(SlotMaterial);
        }
    }

    const USurfaceOpaque* SourceMaterial{ResolveAsset<USurfaceOpaque>(Material.GetHandle())};
    const FAssetEntry* Entry{FindEntry(Material.GetHandle())};

    if (SourceMaterial == nullptr || Entry == nullptr || Entry->mPhysicalPath.extension() != ".material") {
        return Material;
    }

    FAssetReference ResolvedPipeline{Pipeline};

    ResolvedPipeline.Resolve(this);

    const FString PipelineName{std::filesystem::path{ResolvedPipeline.GetPath().mPath}.stem().string().c_str()};
    EMaterialShader Shader{EMaterialShader::Base};

    if (PipelineName == "TexturedBase") {
        Shader = EMaterialShader::Textured;
    } else if (PipelineName == "BaseSolid") {
        Shader = EMaterialShader::BaseSolid;
    } else if (PipelineName == "Alternate") {
        Shader = EMaterialShader::Alternate;
    } else if (PipelineName == "AlternateSolid") {
        Shader = EMaterialShader::AlternateSolid;
    } else if (PipelineName == "SkyDome") {
        Shader = EMaterialShader::Sky;
    } else if (PipelineName == "Gizmo") {
        Shader = EMaterialShader::Gizmo;
    }

    if (SourceMaterial->GetShader() == Shader) {
        return Material;
    }

    const std::filesystem::path Path{Entry->mPhysicalPath.parent_path() / (Entry->mPhysicalPath.stem().string() + ".Shader" + std::to_string(static_cast<Uint32>(Shader)) + ".material")};
    const FAssetHandle Existing{FindAsset(MakeAssetPath(Path))};

    if (Existing) {
        return MakeReference(Existing);
    }

    std::unique_ptr<USurfaceOpaque> Variant{NewObject<USurfaceOpaque>()};

    Variant->SetParameters(SourceMaterial->GetParameters());
    Variant->SetShader(Shader);

    if (!FMaterialAssetSerializer::Save(Path, *Variant)) {
        return Material;
    }

    const FAssetHandle Handle{RegisterImportedAsset(Path, Path, std::move(Variant))};

    return Handle ? MakeReference(Handle) : Material;
}

bool FAssetRegistry::ConvertLegacyAssets() {
    TArray<std::filesystem::path> Sources{};

    for (const std::filesystem::directory_entry& Entry : std::filesystem::recursive_directory_iterator(mContentRoot)) {
        if (Entry.is_regular_file() && Entry.path().extension() == ".mtl") {
            Sources.push_back(Entry.path());
        }
    }

    for (const std::filesystem::path& Source : Sources) {
        TArray<FImportedMaterial> Imported{};

        const FObjImporter::FTextureResolver UnresolvedTexture{[](const std::filesystem::path&) {
            return FAssetReference{};
        }};

        if (!FObjImporter::LoadMaterialFile(Source, UnresolvedTexture, Imported) || Imported.empty()) {
            return false;
        }

        const std::filesystem::path Folder{Source.string() + ".materials"};
        const std::filesystem::path Metadata{MakeSidecarPath(MakeMaterialPath(Folder, Imported.front().mName))};
        std::error_code Error{};

        std::filesystem::create_directories(Folder, Error);

        if (!std::filesystem::exists(Metadata) && std::filesystem::exists(MakeSidecarPath(Source))) {
            std::filesystem::copy_file(MakeSidecarPath(Source), Metadata, Error);
        }

        const TArray<FMeshMaterialSlot> Materials{ImportMaterialLibrary(Source, Folder)};

        if (Error || Materials.empty()) {
            return false;
        }

        if (MakeAssetPath(Source).mPath == "/Game/Material/SkyDome.mtl") {
            USurfaceOpaque* Material{ResolveAsset<USurfaceOpaque>(Materials.front().mMaterial.GetHandle())};

            Material->SetShader(EMaterialShader::Sky);

            if (!SaveMaterial(Materials.front().mMaterial.GetHandle())) {
                return false;
            }
        }
    }

    Sources.clear();

    for (const FAssetEntry& Entry : mAssets) {
        if (Entry.mAssetType == EAssetType::Mesh) {
            Sources.push_back(Entry.mPhysicalPath);
        }
    }

    for (const std::filesystem::path& Source : Sources) {
        FMeshData Existing{};

        if (!FMeshAssetSerializer::Load(Source, Existing)) {
            const FAssetEntry* Entry{FindEntry(FindAsset(MakeAssetPath(Source)))};
            const bool FlipUV{Entry != nullptr && Entry->mMeshMetadata.mFlipUV};

            if (!ImportMeshAsset(Source, Source, FlipUV) || !FMeshAssetSerializer::Load(Source, Existing)) {
                return false;
            }
        }

        for (FMeshMaterialSlot& Slot : Existing.mMaterialSlots) {
            Slot.mMaterial.Resolve(this);

            if (Slot.mMaterial.IsEmpty() && MakeAssetPath(Source).mPath.starts_with("/Game/System/Mesh/")) {
                const FAssetHandle DefaultMaterial{Source.stem() == "SkyDome" ? FindAsset(FAssetPath{"/Game/Material/SkyDome.mtl.materials/M_Material.material"}) : FindAsset(FAssetPath{DefaultStaticMeshMaterialAssetPath})};

                Slot.mMaterial = MakeReference(DefaultMaterial);
            }
        }

        if (!FMeshAssetSerializer::Save(Source, Existing)) {
            return false;
        }

        FAssetEntry* Entry{FindEntry(FindAsset(MakeAssetPath(Source)))};

        if (Entry == nullptr || !LoadMesh(*Entry)) {
            return false;
        }
    }

    return true;
}

FAssetReference FAssetRegistry::MakeReference(FAssetHandle Handle) const {
    FAssetReference Reference{};

    Reference.Assign(Handle, this);
    return Reference;
}

FAssetHandle FAssetRegistry::RegisterImportedAsset(const std::filesystem::path& Source, const std::filesystem::path& Target, std::unique_ptr<UAsset> Asset) {
    if (Asset == nullptr) {
        return {};
    }

    const EAssetType Type{Asset->IsA(UMesh::StaticClass()) ? EAssetType::Mesh : Asset->IsA(UMaterial::StaticClass()) ? EAssetType::Material : EAssetType::Texture};
    FAssetHandle Handle{};

    if (!Target.empty()) {
        Handle = FindAsset(MakeAssetPath(Target));

        if (!Handle && !DiscoverAssetFile(Target)) {
            return {};
        }

        Handle = FindAsset(MakeAssetPath(Target));
    } else {
        Handle = AllocateHandle();

        FAssetEntry Entry{};

        Entry.mHandle = Handle;
        Entry.mPhysicalPath = Source;
        Entry.mAssetType = Type;
        Entry.mAssetPath = FAssetPath{FString{"/Viewer/"} + std::to_string(Handle.mId).c_str() + "/" + Source.filename().generic_string().c_str()};

        if (Handle.mId < mAssets.size()) {
            mAssets[Handle.mId] = std::move(Entry);
        } else {
            mAssets.push_back(std::move(Entry));
        }

        mPathToHandle[mAssets[Handle.mId].mAssetPath] = Handle;
    }

    FAssetEntry* Entry{FindEntry(Handle)};

    if (Entry == nullptr) {
        return {};
    }

    Asset->SetAssetName(Entry->mAssetPath.mPath);
    Entry->mAsset = std::move(Asset);

    return Handle;
}

FAssetHandle FAssetRegistry::ImportTexture(const std::filesystem::path& Source, const std::filesystem::path& TargetFolder) {
    const FAssetHandle Existing{FindAsset(MakeAssetPath(Source))};

    if (Existing) {
        FAssetEntry* Entry{FindEntry(Existing)};

        if (Entry != nullptr && Entry->mAsset == nullptr && !LoadTexture(*Entry)) {
            return {};
        }

        return ResolveAsset<UTexture>(Existing) != nullptr ? Existing : FAssetHandle{};
    }

    for (const FAssetEntry& Entry : mAssets) {
        if (Entry.mPhysicalPath == Source && (TargetFolder.empty() || Entry.mPersistentGuid.IsValid()) && ResolveAsset<UTexture>(Entry.mHandle) != nullptr) {
            return Entry.mHandle;
        }
    }

    if (GetAssetType(Source) != EAssetType::Texture) {
        return {};
    }

    std::filesystem::path Target{};

    if (!TargetFolder.empty()) {
        Target = TargetFolder / (std::to_string(std::hash<std::string>{}(Source.generic_string())) + "_" + Source.filename().string());

        const FAssetHandle Imported{FindAsset(MakeAssetPath(Target))};

        if (ResolveAsset<UTexture>(Imported) != nullptr) {
            return Imported;
        }
    }

    std::unique_ptr<UTexture> Texture{NewObject<UTexture>()};

    if (!Texture->Initialize(Source, false, ETextureFormat::UNORM, true)) {
        return {};
    }

    if (!TargetFolder.empty()) {
        std::error_code Error{};

        std::filesystem::create_directories(TargetFolder, Error);

        if (Error || !std::filesystem::copy_file(Source, Target, std::filesystem::copy_options::none, Error)) {
            return {};
        }
    }

    return RegisterImportedAsset(Source, Target, std::move(Texture));
}

TArray<FMeshMaterialSlot> FAssetRegistry::ImportMaterialLibrary(const std::filesystem::path& Source, const std::filesystem::path& TargetFolder) {
    TArray<FImportedMaterial> Imported{};

    if (!FObjImporter::LoadMaterialFile(Source, [this, &TargetFolder](const std::filesystem::path& TexturePath) {
        const FAssetHandle Texture{ImportTexture(TexturePath, TargetFolder.empty() ? std::filesystem::path{} : TargetFolder / "Textures")};

        return MakeReference(Texture ? Texture : FindAsset(FAssetPath{DefaultCheckerboardTexturePath}));
    }, Imported)) {
        return {};
    }

    TArray<FMeshMaterialSlot> Materials{};

    for (const FImportedMaterial& Item : Imported) {
        std::filesystem::path Target{};

        if (!TargetFolder.empty()) {
            Target = MakeMaterialPath(TargetFolder, Item.mName);
        }

        FAssetHandle Handle{Target.empty() ? FAssetHandle{} : FindAsset(MakeAssetPath(Target))};

        if (!Handle) {
            std::unique_ptr<USurfaceOpaque> Material{NewObject<USurfaceOpaque>()};

            if (!Material->SetParameters(Item.mParameters)) {
                return {};
            }

            if (!Target.empty() && !FMaterialAssetSerializer::Save(Target, *Material)) {
                return {};
            }

            Handle = RegisterImportedAsset(Source, Target, std::move(Material));
        }

        FAssetEntry* Entry{FindEntry(Handle)};

        if (Entry == nullptr || (Entry->mAsset == nullptr && !LoadMaterial(*Entry))) {
            return {};
        }

        Materials.push_back(FMeshMaterialSlot{Item.mName, MakeReference(Handle)});
    }

    return Materials;
}

FAssetHandle FAssetRegistry::ImportMeshAsset(const std::filesystem::path& Source, const std::filesystem::path& Target, bool FlipUV) {
    TArray<FAssetHandle> Existing{};

    for (const FAssetEntry& Entry : mAssets) {
        if (Entry.mAssetType != EAssetType::END) {
            Existing.push_back(Entry.mHandle);
        }
    }

    const auto Rollback{[this, &Existing, &Source, &Target] {
        TArray<FAssetHandle> Added{};

        for (const FAssetEntry& Entry : mAssets) {
            if (Entry.mAssetType != EAssetType::END && std::ranges::find(Existing, Entry.mHandle) == Existing.end()) {
                Added.push_back(Entry.mHandle);
            }
        }

        std::error_code Error{};

        for (FAssetHandle Handle : Added) {
            const FAssetEntry* Entry{FindEntry(Handle)};

            if (!Target.empty() && Entry != nullptr && !Entry->mSidecarPath.empty()) {
                std::filesystem::remove(Entry->mPhysicalPath, Error);
                std::filesystem::remove(Entry->mSidecarPath, Error);
            }

            RemoveAsset(Handle);
        }

        if (!Target.empty() && Source != Target) {
            std::filesystem::remove(Target, Error);
            std::filesystem::remove(MakeSidecarPath(Target), Error);

            const std::filesystem::path Folder{Target.parent_path() / (Target.stem().string() + ".materials")};
            const std::filesystem::path ResolvedFolder{std::filesystem::weakly_canonical(Folder, Error)};
            const std::filesystem::path ResolvedContent{std::filesystem::weakly_canonical(mContentRoot, Error)};
            const std::filesystem::path Relative{ResolvedFolder.lexically_relative(ResolvedContent)};

            if (!Error && !Relative.empty() && *Relative.begin() != ".." && ResolvedFolder.parent_path() == std::filesystem::weakly_canonical(Target.parent_path(), Error)) {
                std::filesystem::remove_all(ResolvedFolder, Error);
            }
        }

        return FAssetHandle{};
    }};

    FMeshData Data{};

    if (!FMeshAssetSerializer::Load(Source, Data)) {
        if (GetLowercaseExtension(Source) == ".bin") {
            std::error_code Error{};
            const bool HasMetadata{std::filesystem::is_regular_file(MakeSidecarPath(Source), Error)};

            if (Error) {
                return Rollback();
            }

            if (HasMetadata) {
                FAssetEntry Metadata{};

                if (!LoadOrCreateMetadata(MakeSidecarPath(Source), EAssetType::Mesh, Metadata)) {
                    return Rollback();
                }

                FlipUV = Metadata.mMeshMetadata.mFlipUV;
            }
        }

        FObjImporter Importer{};
        TMap<std::filesystem::path, TArray<FMeshMaterialSlot>> Libraries{};
        bool MaterialImportFailed{};

        if (!Importer.ImportMesh(Source, FlipUV, [this, &Libraries, &Target, &Source, &MaterialImportFailed](const std::filesystem::path& MaterialPath, FName Name) {
            auto Position{Libraries.find(MaterialPath)};

            if (Position == Libraries.end()) {
                const std::filesystem::path Folder{Target.empty() ? std::filesystem::path{} : Target == Source ? std::filesystem::path{MaterialPath.string() + ".materials"} : Target.parent_path() / (Target.stem().string() + ".materials")};

                Position = Libraries.emplace(MaterialPath, ImportMaterialLibrary(MaterialPath, Folder)).first;

                if (Position->second.empty() && std::filesystem::is_regular_file(MaterialPath)) {
                    MaterialImportFailed = true;
                }
            }

            const auto Material{std::ranges::find(Position->second, Name, &FMeshMaterialSlot::mName)};

            return Material != Position->second.end() ? Material->mMaterial : Name.IsNone() && !Position->second.empty() ? Position->second.front().mMaterial : FAssetReference{};
        }, Data) || MaterialImportFailed) {
            return Rollback();
        }
    }

    std::unique_ptr<UMesh> Mesh{CreateMesh(Data)};

    if (Mesh == nullptr) {
        return Rollback();
    }

    if (!Target.empty()) {
        std::error_code Error{};

        std::filesystem::create_directories(Target.parent_path(), Error);

        if (Error || !FMeshAssetSerializer::Save(Target, Data)) {
            return Rollback();
        }
    }

    const FAssetHandle Handle{RegisterImportedAsset(Source, Target, std::move(Mesh))};

    return Handle ? Handle : Rollback();
}

void FAssetRegistry::Reset() {
    if (AssetRegistryInstance == this) {
        ++AssetRegistryGeneration;
    }

    mAssets.clear();
    mFreeHandles.clear();
    mPathToHandle.clear();
    mGuidToHandle.clear();
    mContentRoot.clear();
}

void FAssetRegistry::Finalize() {
    for (FAssetEntry& Entry : mAssets) {
        if (Entry.mAsset == nullptr || !Entry.mAsset->GetClass()->IsChildOf(UMaterial::StaticClass())) {
            continue;
        }

        UMaterial* Material{static_cast<UMaterial*>(Entry.mAsset.get())};

        Material->Finalize(this);
    }
}

FAssetHandle FAssetRegistry::EnsureDefaultStaticMeshMaterial() const {
    const FAssetHandle DefaultMaterialHandle{FindAsset(FAssetPath{DefaultStaticMeshMaterialAssetPath})};

    if (ResolveAsset<UMaterial>(DefaultMaterialHandle) == nullptr) {
        return {};
    }

    return DefaultMaterialHandle;
}

std::filesystem::path FAssetRegistry::ResolveContentFolder(const FString& VirtualFolder) const {
    constexpr std::string_view VirtualRoot{"/Game"};

    if (VirtualFolder == VirtualRoot) {
        return mContentRoot;
    }

    constexpr std::string_view VirtualPrefix{"/Game/"};

    if (!VirtualFolder.starts_with(VirtualPrefix)) {
        return {};
    }

    const FString RelativeFolder{VirtualFolder.substr(VirtualPrefix.size())};

    const std::filesystem::path Folder{(mContentRoot / std::filesystem::path(RelativeFolder)).lexically_normal()};
    const std::filesystem::path Relative{Folder.lexically_relative(mContentRoot)};

    return !Relative.empty() && *Relative.begin() != ".." ? Folder : std::filesystem::path{};
}

bool FAssetRegistry::EnsureSystemAssets() {
    struct FSystemMeshDefinition {
        const char* mAssetPath{};
    };

    constexpr std::array SystemMeshes{FSystemMeshDefinition{"/Game/System/Mesh/Capsule.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/Cone.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/Cube.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/Cylinder.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/GizmoTorus.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/Plane.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/Pyramid.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/SkyDome.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/Sphere.bin"}, FSystemMeshDefinition{"/Game/System/Mesh/Torus.bin"}};

    for (const FSystemMeshDefinition& Definition : SystemMeshes) {
        const FAssetHandle Handle{FindAsset(FAssetPath{Definition.mAssetPath})};

        if (ResolveAsset<UMesh>(Handle) == nullptr) {
            return false;
        }
    }

    struct FSystemAssetDefinition {
        const char* mAssetPath{};
    };

    constexpr std::array SystemGizmoMaterials{FSystemAssetDefinition{"/Game/System/Material/Red.mtl.materials/M_Red.material"}, FSystemAssetDefinition{"/Game/System/Material/Green.mtl.materials/M_Green.material"}, FSystemAssetDefinition{"/Game/System/Material/Blue.mtl.materials/M_Blue.material"}};

    for (const FSystemAssetDefinition& Definition : SystemGizmoMaterials) {
        const FAssetHandle Handle{FindAsset(FAssetPath{Definition.mAssetPath})};

        if (ResolveAsset<UMaterial>(Handle) == nullptr) {
            return false;
        }
    }

    const FAssetHandle GizmoPipelineHandle{FindAsset(FAssetPath{GizmoPipelineAssetPath})};

    if (ResolveAsset<UPipeline>(GizmoPipelineHandle) == nullptr) {
        return false;
    }

    return true;
}

bool FAssetRegistry::DiscoverAssetFile(const std::filesystem::path& FilePath) {
    const EAssetType AssetType{GetAssetType(FilePath)};

    if (AssetType == EAssetType::END) {
        return true;
    }

    const std::filesystem::path SidecarPath{MakeSidecarPath(FilePath)};
    FAssetEntry Entry{};

    if (!LoadOrCreateMetadata(SidecarPath, AssetType, Entry)) {
        return false;
    }

    if (AssetType == EAssetType::Pipeline && IsPipelineFamilyUnit(FilePath)) {
        const std::filesystem::path FamilyDirectory{FilePath.parent_path()};
        const std::filesystem::path FirstUnitPath{FindFirstPipelineFamilyUnit(FamilyDirectory)};

        if (FirstUnitPath.empty()) {
            return false;
        }

        if (FilePath.lexically_normal() != FirstUnitPath) {
            return true;
        }

        return RegisterDiscoveredAsset(MakeAssetPath(FamilyDirectory), FamilyDirectory, SidecarPath, Entry.mPersistentGuid, AssetType, Entry);
    }

    return RegisterDiscoveredAsset(MakeAssetPath(FilePath), FilePath, SidecarPath, Entry.mPersistentGuid, AssetType, Entry);
}

bool FAssetRegistry::LoadTexture(FAssetEntry& Entry) {
    std::unique_ptr<UTexture> Texture{NewObject<UTexture>()};

    Texture->SetAssetName(Entry.mAssetPath.mPath);

    if (!Texture->Initialize(Entry.mPhysicalPath, Entry.mTextureMetadata.mMakeDDS, ETextureFormat::UNORM, Entry.mTextureMetadata.mGenerateMipMap)) {
        return false;
    }

    Entry.mAsset = std::move(Texture);

    return true;
}

bool FAssetRegistry::LoadFont(FAssetEntry& Entry) {
    std::unique_ptr<UFreeTypeFont> Font{NewObject<UFreeTypeFont>()};

    Font->SetAssetName(Entry.mAssetPath.mPath);

    if (!Font->Initialize(Entry.mPhysicalPath)) {
        return false;
    }

    Entry.mAsset = std::move(Font);

    return true;
}

bool FAssetRegistry::LoadPipeline(FAssetEntry& Entry) {
    std::unique_ptr<UPipeline> Pipeline{NewObject<UPipeline>()};

    Pipeline->SetAssetName(Entry.mAssetPath.mPath);

    const bool BInitialized{Pipeline->Initialize(Entry.mPhysicalPath)};

    if (!BInitialized) {
        return false;
    }

    Entry.mAsset = std::move(Pipeline);

    return true;
}

bool FAssetRegistry::LoadMaterial(FAssetEntry& Entry) {
    std::unique_ptr<USurfaceOpaque> Material{NewObject<USurfaceOpaque>()};

    if (!FMaterialAssetSerializer::Load(Entry.mPhysicalPath, *Material)) {
        return false;
    }

    Material->SetAssetName(Entry.mAssetPath.mPath);
    Material->Finalize(this);
    Entry.mAsset = std::move(Material);

    return true;
}

std::unique_ptr<UMesh> FAssetRegistry::CreateMesh(FMeshData& Data) {
    for (FMeshMaterialSlot& Slot : Data.mMaterialSlots) {
        Slot.mMaterial.Resolve(this);
    }

    std::unique_ptr<UMesh> Mesh{NewObject<UMesh>()};

    if (!Mesh->Initialize(Data)) {
        return nullptr;
    }

    for (Uint32 Level{1}; Level < GLODCount; ++Level) {
        Mesh->GenerateLOD(Level, GLODSettings[Level].mTargetRatio);
    }

    return Mesh;
}

bool FAssetRegistry::LoadMesh(FAssetEntry& Entry) {
    FMeshData Data{};

    if (!FMeshAssetSerializer::Load(Entry.mPhysicalPath, Data)) {
        return false;
    }

    std::unique_ptr<UMesh> Mesh{CreateMesh(Data)};

    if (Mesh == nullptr) {
        return false;
    }

    Mesh->SetAssetName(Entry.mAssetPath.mPath);

    Entry.mAsset = std::move(Mesh);

    return true;
}

bool FAssetRegistry::RegisterDiscoveredAsset(const FAssetPath& AssetPath, const std::filesystem::path& PhysicalPath, const std::filesystem::path& SidecarPath, const FGuid& PersistentGuid, EAssetType AssetType, FAssetEntry& Entry) {
    if (!AssetPath || !PersistentGuid.IsValid() || AssetType == EAssetType::END || mPathToHandle.contains(AssetPath) || mGuidToHandle.contains(PersistentGuid)) {
        return false;
    }

    const FAssetHandle Handle{AllocateHandle()};

    Entry.mAssetPath = AssetPath;
    Entry.mPhysicalPath = PhysicalPath;
    Entry.mSidecarPath = SidecarPath;
    Entry.mPersistentGuid = PersistentGuid;
    Entry.mAssetType = AssetType;
    Entry.mHandle = Handle;

    if (Handle.mId < mAssets.size()) {
        mAssets[Handle.mId] = std::move(Entry);
    } else {
        mAssets.emplace_back(std::move(Entry));
    }

    mPathToHandle[AssetPath] = Handle;
    mGuidToHandle[PersistentGuid] = Handle;

    return true;
}

std::filesystem::path FAssetRegistry::MakeSidecarPath(const std::filesystem::path& AssetPath) {
    return std::filesystem::path{AssetPath.string() + ".meta"};
}

bool FAssetRegistry::LoadOrCreateMetadata(const std::filesystem::path& SidecarPath, EAssetType AssetType, FAssetEntry& Entry) {
    if (std::filesystem::exists(SidecarPath)) {
        std::ifstream Input{SidecarPath};

        if (!Input.is_open()) {
            return false;
        }

        rapidjson::Document Document{};
        rapidjson::IStreamWrapper Stream{Input};

        Document.ParseStream(Stream);

        if (!Input.good() && !Input.eof()) {
            return false;
        }

        if (!Document.IsObject() || !Document.HasMember("Guid") || !Document["Guid"].IsString()) {
            return false;
        }

        if (!Entry.mPersistentGuid.Parse(Document["Guid"].GetString()) || !Entry.mPersistentGuid.IsValid()) {
            return false;
        }

        if (AssetType == EAssetType::Texture) {
            if (Document.HasMember("TextureFormat")) {
                if (!Document["TextureFormat"].IsBool()) {
                    return false;
                }

                Entry.mTextureMetadata.mMakeDDS = Document["TextureFormat"].GetBool();
            }

            if (Document.HasMember("GenerateMipMap")) {
                if (!Document["GenerateMipMap"].IsBool()) {
                    return false;
                }

                Entry.mTextureMetadata.mGenerateMipMap = Document["GenerateMipMap"].GetBool();
            }
        } else if (AssetType == EAssetType::Mesh && Document.HasMember("FlipUV")) {
            if (!Document["FlipUV"].IsBool()) {
                return false;
            }

            Entry.mMeshMetadata.mFlipUV = Document["FlipUV"].GetBool();
        }

        return true;
    }

    Entry.mPersistentGuid = FGuid::NewGuid();

    if (!Entry.mPersistentGuid.IsValid()) {
        return false;
    }

    rapidjson::Document Document{};

    Document.SetObject();

    rapidjson::Document::AllocatorType& Allocator{Document.GetAllocator()};
    const FString GuidString{Entry.mPersistentGuid.ToString()};

    Document.AddMember("Guid", rapidjson::Value(GuidString.c_str(), Allocator), Allocator);

    if (AssetType == EAssetType::Texture) {
        Document.AddMember("TextureFormat", Entry.mTextureMetadata.mMakeDDS, Allocator);
        Document.AddMember("GenerateMipMap", Entry.mTextureMetadata.mGenerateMipMap, Allocator);
    } else if (AssetType == EAssetType::Mesh) {
        Document.AddMember("FlipUV", Entry.mMeshMetadata.mFlipUV, Allocator);
    }

    return FJsonFile::Save(SidecarPath, Document);
}

bool FAssetRegistry::IsPipelineFamilyUnit(const std::filesystem::path& FilePath) {
    const std::filesystem::path FamilyDirectory{FilePath.parent_path()};

    return FilePath.extension() == ".json" && FamilyDirectory.parent_path().filename() == "Pipeline" && FilePath.stem().generic_string().starts_with(FamilyDirectory.filename().generic_string() + "_");
}

std::filesystem::path FAssetRegistry::FindFirstPipelineFamilyUnit(const std::filesystem::path& FamilyDirectory) {
    std::error_code ErrorCode{};
    std::vector<std::filesystem::path> UnitPaths{};

    for (const std::filesystem::directory_entry& Entry : std::filesystem::directory_iterator(FamilyDirectory, ErrorCode)) {
        if (ErrorCode) {
            return {};
        }

        if (Entry.is_regular_file(ErrorCode) && IsPipelineFamilyUnit(Entry.path())) {
            UnitPaths.emplace_back(Entry.path().lexically_normal());
        }
    }

    if (ErrorCode || UnitPaths.empty()) {
        return {};
    }

    std::ranges::sort(UnitPaths, {}, [](const std::filesystem::path& Path) {
        return Path.filename().generic_string();
    });

    return UnitPaths.front();
}

FAssetPath FAssetRegistry::MakeAssetPath(const std::filesystem::path& PhysicalPath) const {
    std::error_code ErrorCode{};
    std::filesystem::path RelativePath{std::filesystem::relative(PhysicalPath, mContentRoot, ErrorCode).lexically_normal()};

    if (ErrorCode || RelativePath.empty() || *RelativePath.begin() == "..") {
        return {};
    }

    return FAssetPath{FString{"/Game/"} + RelativePath.generic_string().c_str()};
}

EAssetType FAssetRegistry::GetAssetType(const std::filesystem::path& FilePath) {
    const FString Extension{GetLowercaseExtension(FilePath)};

    if (Extension == ".bin") {
        return EAssetType::Mesh;
    }

    if (Extension == ".material") {
        return EAssetType::Material;
    }

    if (Extension == ".png" || Extension == ".jpg" || Extension == ".jpeg" || Extension == ".dds" || Extension == ".tga" || Extension == ".bmp" || Extension == ".tif" || Extension == ".tiff" || Extension == ".gif" || Extension == ".hdr") {
        return EAssetType::Texture;
    }

    if (Extension == ".ttf" || Extension == ".otf") {
        return EAssetType::Font;
    }

    if (Extension == ".json" && std::ranges::any_of(FilePath.parent_path(), [](const std::filesystem::path& PathPart) {
        return PathPart == "Pipeline";
    })) {
        return EAssetType::Pipeline;
    }

    return EAssetType::END;
}

FAssetHandle FAssetRegistry::AllocateHandle() {
    if (!mFreeHandles.empty()) {
        const FAssetHandle Handle{mFreeHandles.back()};

        mFreeHandles.pop_back();
        return Handle;
    }

    return FAssetHandle{static_cast<Uint32>(mAssets.size()), 0};
}

FAssetEntry* FAssetRegistry::FindEntry(FAssetHandle Handle) {
    if (Handle.mId >= mAssets.size()) {
        return nullptr;
    }

    FAssetEntry& Entry{mAssets[Handle.mId]};

    return Entry.mHandle == Handle ? &Entry : nullptr;
}

const FAssetEntry* FAssetRegistry::FindEntry(FAssetHandle Handle) const {
    if (Handle.mId >= mAssets.size()) {
        return nullptr;
    }

    const FAssetEntry& Entry{mAssets[Handle.mId]};

    return Entry.mHandle == Handle ? &Entry : nullptr;
}

void FAssetRegistry::RemoveHandleMappings(FAssetHandle Handle) {
    for (auto It{mPathToHandle.begin()}; It != mPathToHandle.end();) {
        if (It->second == Handle) {
            It = mPathToHandle.erase(It);
        } else {
            ++It;
        }
    }

    for (auto It{mGuidToHandle.begin()}; It != mGuidToHandle.end();) {
        if (It->second == Handle) {
            It = mGuidToHandle.erase(It);
        } else {
            ++It;
        }
    }
}

const std::filesystem::path& FAssetRegistry::GetContentRoot() const {
    return mContentRoot;
}

const TArray<FAssetEntry>& FAssetRegistry::GetAssetEntries() const {
    return mAssets;
}

const UObject* FAssetRegistry::ResolveAssetObject(FAssetHandle Handle) const {
    const FAssetEntry* Entry{FindEntry(Handle)};

    return Entry != nullptr ? Entry->mAsset.get() : nullptr;
}

TArray<FAssetHandle> FAssetRegistry::GetAssetHandles(const UClass& AssetType) const {
    TArray<FAssetHandle> Handles{};

    for (const FAssetEntry& Entry : mAssets) {
        if (Entry.mAsset != nullptr && Entry.mAsset->GetClass()->IsChildOf(&AssetType)) {
            Handles.push_back(Entry.mHandle);
        }
    }

    return Handles;
}

void FAssetRegistry::SetPipelineRenderMode(FAssetHandle Handle, ERenderMode Mode) {
    if (UPipeline * Pipeline{ResolveAsset<UPipeline>(Handle)}) {
        Pipeline->SetRenderMode(Mode);
    }
}

const FFontGlyph* FAssetRegistry::GetOrCreateFontGlyph(FAssetHandle Handle, char32_t CodePoint) {
    UFont* Font{ResolveAsset<UFont>(Handle)};

    return Font != nullptr ? Font->GetOrCreateGlyph(CodePoint) : nullptr;
}
