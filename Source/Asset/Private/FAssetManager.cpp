/**
 * @file FAssetManager.cpp
 * @brief 프로젝트 콘텐츠 경로와 메시·텍스처 에셋의 로드 및 캐시에 대한 구현입니다.
 * @ingroup Asset
 */

#include "Asset/FAssetManager.h"
#include "Asset/AssetStats.h"
#include "Core/FilePath.h"
#include "Asset/FAssetFile.h"
#include "Asset/UStaticMesh.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"
#include "Asset/UTexture2D.h"
#include "Asset/UMaterial.h"
#include "Asset/Sequence/ULevelSequence.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/Animation/UBlendSpace1D.h"
#include "Asset/Animation/UAnimGraph.h"
#include "Core/Base/ErrorHandler.h"
#include "FMaterialFile.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Serialization/JsonArchive.h"
#include "Serialization/MemoryArchive.h"
#include "FAssetSerialization.h"
#include "FTextureAssetData.h"
#include "FAssetImportScope.h"
#include "Asset/Import/IMeshImporter.h"
#include "Asset/Import/FMeshImportResult.h"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

namespace Asset {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 에셋 상대 경로를 대소문자 구분 없이 조회할 캐시 키로 변환합니다.
         * @param[in] Relative 캐시 키로 변환할 프로젝트 상대 에셋 경로입니다.
         * @return 입력 값을 변환한 문자열입니다.
         */
        Core::FString CacheKey(const Core::FString& Relative) {
            return Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(Relative)));
        }

        // 캐시에 올라간 에셋 객체는 바꾸지 않으므로, 같은 경로에 다른 객체가 이미 있으면 저장을 거부합니다.
        template <typename TCache, typename TAsset>
        bool CheckCacheSlot(const TCache& Cache, const Core::FString& Key, const TAsset* Asset, Core::FString& Error) {
            const auto Existing{Cache.find(Key)};

            if (Existing != Cache.end() && Existing->second.get() != Asset) {
                Error = "Another asset is already loaded at this path";

                return false;
            }

            return true;
        }

        /**
         * @brief 파일명 금지 문자·끝 공백과 점·Windows 예약 이름을 정리합니다.
         * @param[in] Name 조회하거나 지정할 이름입니다.
         * @param[in] Fallback 정리한 이름이 비었을 때 사용할 대체 파일 이름입니다.
         * @return 안전하게 정리한 이름이며 입력이 비어 있으면 대체 이름을 사용합니다.
         */
        Core::FString AssetFileName(Core::FString Name, const Core::FString& Fallback) {
            for (char& Character : Name) {
                if (static_cast<unsigned char>(Character) < 32 || Core::FString{"<>:\"/\\|?*"}.find(Character) != Core::FString::npos) {
                    Character = '_';
                }
            }

            while (!Name.empty() && (Name.back() == '.' || Name.back() == ' ')) {
                Name.pop_back();
            }

            if (Name.empty()) {
                Name = Fallback;
            }

            Core::FString Base{Name.substr(0, Name.find('.'))};

            for (char& Character : Base) {
                if (Character >= 'a' && Character <= 'z') {
                    Character = static_cast<char>(Character - 'a' + 'A');
                }
            }

            if (Base == "CON" || Base == "PRN" || Base == "AUX" || Base == "NUL" || (Base.size() == 4 && (Base.starts_with("COM") || Base.starts_with("LPT")) && Base.back() >= '1' && Base.back() <= '9')) {
                Name.insert(Name.begin(), '_');
            }

            return Name;
        }

        /**
         * @brief 기존 파일과 겹치지 않도록 숫자 접미사를 붙인 .masset 경로를 선택합니다.
         * @param[in] Directory 에셋 파일을 만들 폴더입니다.
         * @param[in] Name 조회하거나 지정할 이름입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 선택한 파일 경로이며 파일 시스템 검사 실패 시 빈 경로입니다.
         */
        std::filesystem::path UniqueAssetPath(const std::filesystem::path& Directory, const Core::FString& Name, Core::FString& Error) {
            const auto Base{Core::FilePath::FromUtf8(Name)};
            auto Target{Directory / Base};

            Target += L".masset";

            std::error_code Code{};

            for (Core::Uint32 Index{1}; std::filesystem::exists(Target, Code) && !Code; ++Index) {
                Target = Directory / (Base.wstring() + L"_" + std::to_wstring(Index) + L".masset");
            }

            if (Code) {
                Error = "Cannot choose an asset filename: " + Code.message();

                return {};
            }

            return Target;
        }

        /**
         * @brief 재질 텍스처를 중복 경로 캐시와 공유하여 가져오고 프로젝트 상대 참조를 반환합니다.
         * @param[in,out] Assets 경로 경계를 확인하고 에셋을 가져올 프로젝트 에셋 관리자입니다.
         * @param[in] Texture 가져올 원본 텍스처 참조입니다.
         * @param[in] Directory 가져온 텍스처 에셋을 저장할 폴더입니다.
         * @param[in,out] Textures 원본 경로별로 이미 가져온 텍스처의 프로젝트 참조를 보관하는 캐시입니다.
         * @param[out] ImportedPath 가져온 텍스처의 프로젝트 상대 경로를 기록할 문자열입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 텍스처가 없거나 가져오기에 성공하면 true이며 오류이면 false입니다.
         */
        bool ImportMeshTexture(Asset::FAssetManager& Assets, const Asset::FMeshImportTexture& Texture, const std::filesystem::path& Directory, Core::TMap<Core::FString, Core::FString>& Textures, Core::FString& ImportedPath, Core::FString& Error) {
            ImportedPath.clear();

            if (Texture.GetSource() == Asset::EMeshImportTextureSource::None) {
                return true;
            }

            const bool Embedded{Texture.GetSource() == Asset::EMeshImportTextureSource::Embedded};
            const auto Source{Embedded ? Core::FilePath::FromUtf8(Texture.GetEmbeddedReference()) : Texture.GetFilePath()};
            const Core::FString Key{Core::FString{Embedded ? "Embedded:" : "File:"} + CacheKey(Core::FilePath::ToUtf8(Source))};
            auto Existing{Textures.find(Key)};

            if (Existing == Textures.end()) {
                const auto Target{UniqueAssetPath(Directory, AssetFileName(Core::FilePath::ToUtf8(Source.stem()), "Texture"), Error)};

                if (Target.empty()) {
                    return false;
                }

                bool Imported{};

                if (Embedded) {
                    auto Image{CoreUObject::NewObject<Asset::UTexture2D>()};

                    Imported = Image->LoadFromMemory(Texture.GetEmbeddedData(), Source) && Assets.SaveTexture(Target, *Image, Error);

                    if (!Imported && Error.empty()) {
                        Error = "Cannot decode the embedded texture";
                    }
                } else {
                    Imported = Assets.ImportTexture(Source, Target, Error);
                }

                if (!Imported) {
                    Error = "Cannot import material texture '" + Core::FilePath::ToUtf8(Source) + "': " + Error;

                    return false;
                }

                const Core::FString RelativePath{Assets.GetRelativePath(Target, Error)};

                if (RelativePath.empty()) {
                    return false;
                }

                Existing = Textures.emplace(Key, RelativePath).first;
            }

            ImportedPath = Existing->second;

            return true;
        }

        /**
         * @brief 메시 정점 속성과 UV 채널 및 인덱스를 바이너리로 직렬화합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         * @param[in,out] Mesh 읽거나 직렬화할 메시 데이터입니다.
         */
        void SerializeMesh(Serialization::FMemoryArchive& Archive, Asset::FMeshData& Mesh) {
            Asset::FAssetSerialization::SerializeValues(Archive, Mesh.GetPositions());
            Asset::FAssetSerialization::SerializeValues(Archive, Mesh.GetNormals());
            Asset::FAssetSerialization::SerializeValues(Archive, Mesh.GetTangents());
            Asset::FAssetSerialization::SerializeValues(Archive, Mesh.GetColors());

            Core::Uint32 Channels{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Mesh.GetUVChannels().size())};

            Archive.Serialize("UVChannels", Channels);

            if (Archive.HasError() || Channels > 8) {
                Archive.SetError("Invalid mesh UV channel count");

                return;
            }

            if (Archive.IsLoading()) {
                Mesh.GetUVChannels().resize(Channels);
            }

            for (auto& Channel : Mesh.GetUVChannels()) {
                Asset::FAssetSerialization::SerializeValues(Archive, Channel);
            }

            Asset::FAssetSerialization::SerializeValues(Archive, Mesh.GetIndices());

            if (Archive.HasError() || (Archive.IsLoading() && Archive.GetRemainingBytes() == 0)) {
                return;
            }

            Core::Uint32 ExtensionVersion{1};

            Archive.Serialize("SectionsVersion", ExtensionVersion);

            if (ExtensionVersion != 1) {
                Archive.SetError("Unsupported mesh section version");

                return;
            }

            Core::Uint32 SlotCount{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Mesh.GetMaterialSlots().size())};

            if (!Archive.BeginArray("MaterialSlots", SlotCount) || (Archive.IsLoading() && SlotCount > Archive.GetRemainingBytes() / 8)) {
                Archive.SetError("Invalid mesh material slot count");

                return;
            }

            if (Archive.IsLoading()) {
                Mesh.GetMaterialSlots().clear();
            }

            for (Core::Uint32 Index{}; Index < SlotCount && !Archive.HasError(); ++Index) {
                Core::FString Name{Archive.IsLoading() ? Core::FString{} : Mesh.GetMaterialSlots()[Index].GetName()};
                Core::FString Path{Archive.IsLoading() ? Core::FString{} : Mesh.GetMaterialSlots()[Index].GetMaterialPath()};

                Archive.Serialize("Name", Name);
                Archive.Serialize("Material", Path);

                if (Archive.IsLoading()) {
                    Mesh.GetMaterialSlots().emplace_back(std::move(Name), std::move(Path));
                }
            }

            Archive.EndArray();

            Core::Uint32 SectionCount{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Mesh.GetSubMeshes().size())};

            if (!Archive.BeginArray("SubMeshes", SectionCount) || (Archive.IsLoading() && SectionCount > Archive.GetRemainingBytes() / 12)) {
                Archive.SetError("Invalid submesh count");

                return;
            }

            if (Archive.IsLoading()) {
                Mesh.GetSubMeshes().clear();
            }

            for (Core::Uint32 Index{}; Index < SectionCount && !Archive.HasError(); ++Index) {
                Core::Uint32 First{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetFirstIndex()};
                Core::Uint32 Count{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetIndexCount()};
                Core::Uint32 Slot{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetMaterialIndex()};

                Archive.Serialize("FirstIndex", First);
                Archive.Serialize("IndexCount", Count);
                Archive.Serialize("MaterialIndex", Slot);

                if (Archive.IsLoading()) {
                    Mesh.GetSubMeshes().emplace_back(First, Count, Slot);
                }
            }

            Archive.EndArray();
        }

        void SerializeBones(Serialization::FMemoryArchive& Archive, Core::TArray<Asset::FMeshImportBone>& Bones) {
            Core::Uint32 BoneCount{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Bones.size())};

            if (Archive.HasError() || !Archive.BeginArray("Bones", BoneCount) || BoneCount == 0 || BoneCount > Asset::MaxSkeletonBones || (Archive.IsLoading() && BoneCount > Archive.GetRemainingBytes() / 72)) {
                Archive.SetError("Invalid skeleton bone count");

                return;
            }

            if (Archive.IsLoading()) {
                Bones.clear();
                Bones.reserve(BoneCount);
            }

            for (Core::Uint32 Index{}; Index < BoneCount && !Archive.HasError(); ++Index) {
                Core::FString Name{Archive.IsLoading() ? Core::FString{} : Bones[Index].GetName()};
                Core::Int32 Parent{Archive.IsLoading() ? -1 : Bones[Index].GetParentIndex()};
                Core::FMatrix BindPose{Archive.IsLoading() ? Core::FMatrix{} : Bones[Index].GetBindPose()};

                Archive.Serialize("Name", Name);
                Archive.Serialize("ParentIndex", Parent);
                Archive.Serialize("BindPose", BindPose);

                if (Archive.IsLoading() && !Archive.HasError()) {
                    Bones.emplace_back(std::move(Name), Parent, BindPose);
                }
            }

            Archive.EndArray();
        }

        void SerializeSkeleton(Serialization::FMemoryArchive& Archive, Core::TArray<Asset::FMeshImportBone>& Bones) {
            Core::Uint32 Version{1};

            Archive.Serialize("SkeletonVersion", Version);

            if (Archive.HasError() || Version != 1) {
                Archive.SetError("Unsupported skeleton version");

                return;
            }

            SerializeBones(Archive, Bones);
        }

        void SerializeAnimSequence(Serialization::FMemoryArchive& Archive, Asset::FAnimDataModel& Model, bool& EnableRootMotion, Asset::ERootMotionRootLock& RootLock, bool& ForceRootLock, Core::FString& RootBone) {
            Core::Uint32 Version{2};

            Archive.Serialize("AnimSequenceVersion", Version);

            if (Archive.HasError() || (Version != 1 && Version != 2)) {
                Archive.SetError("Unsupported animation sequence version");

                return;
            }

            Asset::FFrameRate FrameRate{Model.GetFrameRate()};
            Core::Uint32 Frames{Model.GetNumberOfFrames()};

            Archive.Serialize("FrameRateNumerator", FrameRate.mNumerator);
            Archive.Serialize("FrameRateDenominator", FrameRate.mDenominator);
            Archive.Serialize("NumberOfFrames", Frames);

            if (Archive.HasError() || !FrameRate.IsValid() || Frames == 0 || Frames > Asset::FAnimDataModel::MaxFrames) {
                Archive.SetError("Invalid animation sequence timing");

                return;
            }

            const std::size_t Keys{static_cast<std::size_t>(Frames) + 1};
            auto& Tracks{Model.GetBoneAnimationTracks()};
            Core::Uint32 TrackCount{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Tracks.size())};

            // 트랙 하나는 최소한 이름 길이와 세 채널의 키 수, 키 하나씩(12 + 16 + 12바이트)을 가집니다.
            constexpr std::size_t MinimumTrackBytes{sizeof(Core::Uint32) * 4 + 40};

            if (!Archive.BeginArray("Tracks", TrackCount) || TrackCount == 0 || TrackCount > Asset::FAnimDataModel::MaxTracks || (Archive.IsLoading() && TrackCount > Archive.GetRemainingBytes() / MinimumTrackBytes)) {
                Archive.SetError("Invalid animation sequence track count");

                return;
            }

            if (Archive.IsLoading()) {
                Tracks.assign(TrackCount, {});
            }

            // 채널마다 키 수를 먼저 기록하며, 키 수는 1 또는 프레임 수 + 1만 허용합니다.
            const auto SerializeChannel{[&Archive, Keys](std::string_view Name, auto& Channel, std::size_t KeyBytes) {
                Core::Uint32 Count{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Channel.size())};

                if (!Archive.BeginArray(Name, Count) || (Count != 1 && Count != Keys) || (Archive.IsLoading() && Count > Archive.GetRemainingBytes() / KeyBytes)) {
                    Archive.SetError("Invalid animation sequence key count");

                    return;
                }

                if (Archive.IsLoading()) {
                    Channel.resize(Count);
                }

                for (auto& Key : Channel) {
                    if (Archive.HasError()) {
                        break;
                    }

                    Archive.Serialize("Key", Key);
                }

                Archive.EndArray();
            }};

            for (Asset::FBoneAnimationTrack& Track : Tracks) {
                if (Archive.HasError()) {
                    break;
                }

                Archive.Serialize("Name", Track.mName);
                SerializeChannel("PosKeys", Track.mInternalTrack.mPosKeys, sizeof(float) * 3);
                SerializeChannel("RotKeys", Track.mInternalTrack.mRotKeys, sizeof(float) * 4);
                SerializeChannel("ScaleKeys", Track.mInternalTrack.mScaleKeys, sizeof(float) * 3);
            }

            Archive.EndArray();

            if (Version >= 2) {
                Core::Uint32 Lock{static_cast<Core::Uint32>(RootLock)};

                Archive.Serialize("EnableRootMotion", EnableRootMotion);
                Archive.Serialize("RootMotionRootLock", Lock);
                Archive.Serialize("ForceRootLock", ForceRootLock);
                Archive.Serialize("RootMotionBone", RootBone);

                if (Lock > static_cast<Core::Uint32>(Asset::ERootMotionRootLock::Zero) || RootBone.find('\0') != Core::FString::npos) {
                    Archive.SetError("Invalid root motion settings");

                    return;
                }

                RootLock = static_cast<Asset::ERootMotionRootLock>(Lock);
            }

            if (Archive.IsLoading()) {
                Model.SetFrameRate(FrameRate);
                Model.SetNumberOfFrames(Frames);
            }
        }

        void SerializeSkeletalMesh(Serialization::FMemoryArchive& Archive, Asset::FMeshData& Mesh, Core::TArray<Asset::FMeshImportSkinWeight>& SkinWeights, Core::TArray<Asset::FMeshImportBone>* LegacyBones = nullptr) {
            Core::Uint32 Version{2};

            Archive.Serialize("SkeletalMeshVersion", Version);

            if (Archive.HasError() || (Version != 2 && (Version != 1 || LegacyBones == nullptr))) {
                Archive.SetError("Unsupported skeletal mesh version");

                return;
            }

            SerializeMesh(Archive, Mesh);

            if (Version == 1) {
                SerializeBones(Archive, *LegacyBones);
            }

            Core::Uint32 VertexCount{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(SkinWeights.size())};

            Archive.Serialize("SkinWeightCount", VertexCount);

            constexpr std::size_t WeightBytes{Asset::FMeshImportSkinWeight::MaxInfluences * (sizeof(Core::Uint32) + sizeof(float))};

            if (Archive.HasError() || VertexCount != Mesh.GetPositions().size() || VertexCount > 16000000 || (Archive.IsLoading() && VertexCount > Archive.GetRemainingBytes() / WeightBytes)) {
                Archive.SetError("Invalid skeletal mesh skin weight count");

                return;
            }

            if (Archive.IsLoading()) {
                SkinWeights.clear();
                SkinWeights.reserve(VertexCount);
            }

            for (Core::Uint32 Vertex{}; Vertex < VertexCount && !Archive.HasError(); ++Vertex) {
                std::array<Core::Uint32, Asset::FMeshImportSkinWeight::MaxInfluences> BoneIndices{Archive.IsLoading() ? std::array<Core::Uint32, Asset::FMeshImportSkinWeight::MaxInfluences>{} : SkinWeights[Vertex].GetBoneIndices()};
                std::array<float, Asset::FMeshImportSkinWeight::MaxInfluences> Weights{Archive.IsLoading() ? std::array<float, Asset::FMeshImportSkinWeight::MaxInfluences>{} : SkinWeights[Vertex].GetWeights()};

                for (std::size_t Influence{}; Influence < Asset::FMeshImportSkinWeight::MaxInfluences; ++Influence) {
                    Archive.Serialize("BoneIndex", BoneIndices[Influence]);
                    Archive.Serialize("Weight", Weights[Influence]);
                }

                if (Archive.IsLoading() && !Archive.HasError()) {
                    SkinWeights.emplace_back(BoneIndices, Weights);
                }
            }
        }

    }

    /**
     * @brief 전달받은 설정과 의존성으로 FAssetManager의 초기 상태를 구성합니다.
     */
    Asset::FAssetManager::FAssetManager(std::filesystem::path ContentRoot)
		: mContentRoot{std::filesystem::absolute(ContentRoot).lexically_normal()} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Asset::FAssetManager::~FAssetManager() = default;

    /**
     * @brief 에셋 경로의 기준이 되는 콘텐츠 폴더를 반환합니다.
     */
    const std::filesystem::path& Asset::FAssetManager::GetContentRoot() const {
        return mContentRoot;
    }

    /**
     * @brief 이 관리자가 보유한 메시의 경로를 조회합니다.
     */
    Core::FString Asset::FAssetManager::GetMeshPath(const Asset::UStaticMesh* Mesh) const {
        for (const auto& Entry : mMeshes) {
            if (Entry.second.get() == Mesh) {
                return Entry.first;
            }
        }

        return {};
    }

    /**
     * @brief 캐시에 등록된 스켈레탈 메시의 프로젝트 상대 경로를 반환합니다.
     */
    Core::FString Asset::FAssetManager::GetSkeletalMeshPath(const Asset::USkeletalMesh* Mesh) const {
        for (const auto& Entry : mSkeletalMeshes) {
            if (Entry.second.get() == Mesh) {
                return Entry.first;
            }
        }

        return {};
    }

    /**
     * @brief 이 관리자의 캐시에서 텍스처에 대응하는 에셋 경로를 조회합니다.
     */
    Core::FString Asset::FAssetManager::GetTexturePath(const Asset::UTexture2D* Texture) const {
        for (const auto& Entry : mTextures) {
            if (Entry.second.get() == Texture) {
                return Entry.first;
            }
        }

        return {};
    }

    /**
     * @brief 콘텐츠 폴더를 벗어나는 경로를 거부하고 정규화된 절대 경로를 반환합니다.
     */
    std::filesystem::path Asset::FAssetManager::ResolvePath(const std::filesystem::path& Path, Core::FString& Error) const {
        Error.clear();

        if (Path.empty() || (Path.has_root_name() && !Path.is_absolute())) {
            Error = "An asset path is required";

            return {};
        }

        std::error_code Code{};
        const auto Root{std::filesystem::weakly_canonical(mContentRoot, Code)};
        const auto Resolved{Code ? std::filesystem::path{} : std::filesystem::weakly_canonical(Path.is_absolute() ? Path : Root / Path, Code)};

        if (Code || Root.empty() || Resolved.empty()) {
            Error = "Cannot resolve the project asset path";

            return {};
        }

        auto RootPart{Root.begin()};
        auto PathPart{Resolved.begin()};

        for (; RootPart != Root.end() && PathPart != Resolved.end(); ++RootPart, ++PathPart) {
            if (_wcsicmp(RootPart->c_str(), PathPart->c_str()) != 0) {
                break;
            }
        }

        if (RootPart != Root.end() || PathPart == Resolved.end()) {
            Error = "Assets must be inside the current project's Content folder";

            return {};
        }

        return Resolved;
    }

    /**
     * @brief 경로가 콘텐츠 폴더 안에 있는지 검증하고 UTF-8 상대 경로를 반환합니다.
     */
    Core::FString Asset::FAssetManager::GetRelativePath(const std::filesystem::path& Path, Core::FString& Error) const {
        const auto Resolved{ResolvePath(Path, Error)};

        if (Resolved.empty()) {
            return {};
        }

        std::error_code Code{};
        const auto Relative{std::filesystem::relative(Resolved, mContentRoot, Code)};

        if (Code) {
            Error = "Cannot make a project relative asset path";

            return {};
        }

        return Core::FilePath::ToGenericUtf8(Relative);
    }

    /**
     * @brief 콘텐츠 폴더 아래의 .masset 파일을 찾아 상대 경로순으로 반환합니다.
     */
    Core::TArray<std::filesystem::path> Asset::FAssetManager::GetAssetPaths(Core::FString& Error) const {
        Error.clear();

        Core::TArray<std::filesystem::path> Paths{};
        std::error_code Code{};
        std::filesystem::recursive_directory_iterator Iterator{mContentRoot, Code};
        const std::filesystem::recursive_directory_iterator End{};

        while (!Code && Iterator != End) {
            if (Iterator->is_regular_file(Code) && Iterator->path().extension() == L".masset") {
                const auto Resolved{ResolvePath(Iterator->path(), Error)};

                if (Resolved.empty()) {
                    return {};
                }

                Paths.push_back(Resolved.lexically_relative(mContentRoot));
            }

            Iterator.increment(Code);
        }

        if (Code) {
            Error = "Cannot enumerate project assets: " + Code.message();
        }

        std::sort(Paths.begin(), Paths.end());

        return Paths;
    }

    /**
     * @brief 메시 에셋을 캐시에서 조회하거나 파일에서 로드하여 보관합니다.
     */
    Asset::UStaticMesh* Asset::FAssetManager::LoadMesh(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Asset::AssetStats::MeshLoad};
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mMeshes.find(Key)};

        if (Existing != mMeshes.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(ResolvePath(Path, Error), Asset::EAssetType::Mesh, Payload, Error)) {
            return nullptr;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Asset::FMeshData Data{};

        SerializeMesh(Archive, Data);

        auto Mesh{CoreUObject::NewObject<Asset::UStaticMesh>()};

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0 || !Mesh->SetMeshData(std::move(Data))) {
            Error = Archive.HasError() ? Archive.GetError() : "Invalid mesh asset data";

            return nullptr;
        }

        Asset::UStaticMesh* Result{Mesh.get()};

        Mesh->SetName(Core::FilePath::ToUtf8(Path.stem()));
        mMeshes.emplace(Key, std::move(Mesh));

        return Result;
    }

    /**
     * @brief 스켈레탈 메시 파일에서 본·스킨 가중치·기하 데이터를 읽어 캐시에 등록합니다.
     */
    Asset::USkeletalMesh* Asset::FAssetManager::LoadSkeletalMesh(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Asset::AssetStats::MeshLoad};
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mSkeletalMeshes.find(Key)};

        if (Existing != mSkeletalMeshes.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(ResolvePath(Path, Error), Asset::EAssetType::SkeletalMesh, Payload, Error)) {
            return nullptr;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Asset::FMeshData Data{};
        Core::TArray<Asset::FMeshImportBone> LegacyBones{};
        Core::TArray<Asset::FMeshImportSkinWeight> SkinWeights{};

        SerializeSkeletalMesh(Archive, Data, SkinWeights, &LegacyBones);

        auto Mesh{CoreUObject::NewObject<Asset::USkeletalMesh>()};

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0 || !Mesh->SetMeshData(std::move(Data), std::move(SkinWeights))) {
            Error = Archive.HasError() ? Archive.GetError() : "Invalid skeletal mesh asset data";

            return nullptr;
        }

        Asset::USkeletalMesh* Result{Mesh.get()};

        Mesh->SetName(Core::FilePath::ToUtf8(Path.stem()));

        if (!LegacyBones.empty()) {
            auto Skeleton{CoreUObject::NewObject<Asset::USkeleton>()};

            if (!Skeleton->SetBones(std::move(LegacyBones))) {
                Error = "Invalid embedded skeleton data";

                return nullptr;
            }

            Skeleton->SetName(Mesh->GetName());
            mLegacySkeletons.emplace(Key, std::move(Skeleton));
        }

        mSkeletalMeshes.emplace(Key, std::move(Mesh));

        return Result;
    }

    Asset::USkeleton* Asset::FAssetManager::LoadSkeleton(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mSkeletons.find(Key)};

        if (Existing != mSkeletons.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(ResolvePath(Path, Error), Asset::EAssetType::Skeleton, Payload, Error)) {
            return nullptr;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Core::TArray<Asset::FMeshImportBone> Bones{};

        SerializeSkeleton(Archive, Bones);

        auto Skeleton{CoreUObject::NewObject<Asset::USkeleton>()};

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0 || !Skeleton->SetBones(std::move(Bones))) {
            Error = Archive.HasError() ? Archive.GetError() : "Invalid skeleton asset data";

            return nullptr;
        }

        Asset::USkeleton* Result{Skeleton.get()};

        Skeleton->SetName(Core::FilePath::ToUtf8(Path.stem()));
        mSkeletons.emplace(Key, std::move(Skeleton));

        return Result;
    }

    Asset::USkeleton* Asset::FAssetManager::FindLegacySkeleton(const std::filesystem::path& MeshPath) const {
        Core::FString Error{};
        const auto Existing{mLegacySkeletons.find(CacheKey(GetRelativePath(MeshPath, Error)))};

        return Existing != mLegacySkeletons.end() ? Existing->second.get() : nullptr;
    }

    Asset::UAnimSequence* Asset::FAssetManager::LoadAnimSequence(const std::filesystem::path& Path, Core::FString& Error) {
        Core::FString RelativePath{GetRelativePath(Path, Error)};
        const Core::FString Key{CacheKey(RelativePath)};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mAnimSequences.find(Key)};

        if (Existing != mAnimSequences.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(ResolvePath(Path, Error), Asset::EAssetType::AnimSequence, Payload, Error)) {
            return nullptr;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Asset::FAnimDataModel Model{};
        bool EnableRootMotion{};
        Asset::ERootMotionRootLock RootLock{Asset::ERootMotionRootLock::AnimFirstFrame};
        bool ForceRootLock{};
        Core::FString RootBone{};

        SerializeAnimSequence(Archive, Model, EnableRootMotion, RootLock, ForceRootLock, RootBone);

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
            Error = Archive.HasError() ? Archive.GetError() : "Invalid animation sequence asset data";

            return nullptr;
        }

        auto Sequence{CoreUObject::NewObject<Asset::UAnimSequence>()};

        if (!Sequence->SetDataModel(std::move(Model), Error)) {
            return nullptr;
        }

        Asset::UAnimSequence* Result{Sequence.get()};

        Sequence->SetRootMotionEnabled(EnableRootMotion);
        Sequence->SetRootMotionRootLock(RootLock);
        Sequence->SetForceRootLock(ForceRootLock);
        Sequence->SetRootMotionBone(RootBone);
        Sequence->SetName(Core::FilePath::ToUtf8(Path.stem()));
        Sequence->mAssetPath = std::move(RelativePath);
        mAnimSequences.emplace(Key, std::move(Sequence));

        return Result;
    }

    /**
     * @brief 텍스처 에셋을 캐시에서 조회하거나 파일에서 로드하여 보관합니다.
     */
    Asset::UTexture2D* Asset::FAssetManager::LoadTexture(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Asset::AssetStats::TextureLoad};
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mTextures.find(Key)};

        if (Existing != mTextures.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        const auto Source{ResolvePath(Path, Error)};

        if (Source.empty()) {
            return nullptr;
        }

        if (Source.extension() != L".masset") {
            auto Texture{CoreUObject::NewObject<Asset::UTexture2D>()};

            if (!Texture->LoadFromFile(Source)) {
                Error = "Cannot load the material texture";

                return nullptr;
            }

            Asset::UTexture2D* Result{Texture.get()};

            mTextures.emplace(Key, std::move(Texture));

            return Result;
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(ResolvePath(Path, Error), Asset::EAssetType::Texture, Payload, Error)) {
            return nullptr;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Asset::FTextureAssetData Data{};
        Data.Serialize(Archive);

        auto Texture{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
            Error = Archive.HasError() ? Archive.GetError() : "Invalid texture asset data";

            return nullptr;
        }

        if (!Data.ApplyTo(*Texture, Error)) {
            return nullptr;
        }

        Asset::UTexture2D* Result{Texture.get()};

        mTextures.emplace(Key, std::move(Texture));

        return Result;
    }

    /**
     * @brief 검증한 CPU 메시 데이터를 바이너리 에셋 파일로 저장합니다.
     */
    bool Asset::FAssetManager::SaveMesh(const std::filesystem::path& Path, const Asset::UStaticMesh& Mesh, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty() || !Mesh.IsValid()) {
            if (Error.empty()) {
                Error = "Cannot save an invalid mesh";
            }

            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Asset::FMeshData Data{Mesh.GetMeshData()};

        SerializeMesh(Archive, Data);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        if (!CheckCacheSlot(mMeshes, CacheKey(GetRelativePath(Path, Error)), &Mesh, Error)) {
            return false;
        }

        if (!Asset::FAssetFile::Save(Target, Asset::EAssetType::Mesh, Payload, Error)) {
            return false;
        }

        return true;
    }

    /**
     * @brief 스켈레탈 메시의 기하·본 계층·스킨 가중치를 에셋 파일에 저장합니다.
     */
    bool Asset::FAssetManager::SaveSkeletalMesh(const std::filesystem::path& Path, const Asset::USkeletalMesh& Mesh, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty() || !Mesh.IsValid()) {
            if (Error.empty()) {
                Error = "Cannot save an invalid skeletal mesh";
            }

            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Asset::FMeshData Data{Mesh.GetMeshData()};
        Core::TArray<Asset::FMeshImportSkinWeight> SkinWeights{Mesh.GetSkinWeights()};

        SerializeSkeletalMesh(Archive, Data, SkinWeights);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        if (!CheckCacheSlot(mSkeletalMeshes, CacheKey(GetRelativePath(Path, Error)), &Mesh, Error)) {
            return false;
        }

        if (!Asset::FAssetFile::Save(Target, Asset::EAssetType::SkeletalMesh, Payload, Error)) {
            return false;
        }

        return true;
    }

    bool Asset::FAssetManager::SaveSkeleton(const std::filesystem::path& Path, const Asset::USkeleton& Skeleton, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty() || !Skeleton.IsValid()) {
            if (Error.empty()) {
                Error = "Cannot save an invalid skeleton";
            }

            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Core::TArray<Asset::FMeshImportBone> Bones{Skeleton.GetBones()};

        SerializeSkeleton(Archive, Bones);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        if (!CheckCacheSlot(mSkeletons, CacheKey(GetRelativePath(Path, Error)), &Skeleton, Error)) {
            return false;
        }

        if (!Asset::FAssetFile::Save(Target, Asset::EAssetType::Skeleton, Payload, Error)) {
            return false;
        }

        return true;
    }

    bool Asset::FAssetManager::SaveAnimSequence(const std::filesystem::path& Path, const Asset::UAnimSequence& Sequence, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty() || !Sequence.IsValid()) {
            if (Error.empty()) {
                Error = "Cannot save an invalid animation sequence";
            }

            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Asset::FAnimDataModel Model{Sequence.GetDataModel()};
        bool EnableRootMotion{Sequence.IsRootMotionEnabled()};
        Asset::ERootMotionRootLock RootLock{Sequence.GetRootMotionRootLock()};
        bool ForceRootLock{Sequence.IsRootLockForced()};
        Core::FString RootBone{Sequence.GetRootMotionBone()};

        SerializeAnimSequence(Archive, Model, EnableRootMotion, RootLock, ForceRootLock, RootBone);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        if (!CheckCacheSlot(mAnimSequences, CacheKey(GetRelativePath(Path, Error)), &Sequence, Error)) {
            return false;
        }

        if (!Asset::FAssetFile::Save(Target, Asset::EAssetType::AnimSequence, Payload, Error)) {
            return false;
        }

        return true;
    }

    /**
     * @brief 텍스처의 크기·형식·색 공간과 이미지 데이터를 에셋 파일로 저장합니다.
     */
    bool Asset::FAssetManager::SaveTexture(const std::filesystem::path& Path, const Asset::UTexture2D& Texture, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty() || !Texture.IsValid() || Texture.GetPixels().empty()) {
            if (Error.empty()) {
                Error = "Only textures with stored pixel data can be saved";
            }

            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Asset::FTextureAssetData Data{Texture};
        Data.Serialize(Archive);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        if (!CheckCacheSlot(mTextures, CacheKey(GetRelativePath(Path, Error)), &Texture, Error)) {
            return false;
        }

        if (!Asset::FAssetFile::Save(Target, Asset::EAssetType::Texture, Payload, Error)) {
            return false;
        }

        return true;
    }

    /**
     * @brief 기본 임포트 옵션으로 외부 에셋을 새 폴더에 가져오고 생성한 에셋 경로를 반환합니다.
     */
    bool Asset::FAssetManager::ImportAsset(const std::filesystem::path& Source, const std::filesystem::path& Directory, std::filesystem::path& ImportedPath, Core::FString& Error) {
        return ImportAsset(Source, Directory, Asset::FAssetImportOption{}, ImportedPath, Error);
    }

    /**
     * @brief 임포트 옵션을 따라 외부 에셋을 새 폴더에 가져오고 생성한 주 에셋 경로를 반환합니다.
     */
    bool Asset::FAssetManager::ImportAsset(const std::filesystem::path& Source, const std::filesystem::path& Directory, const Asset::FAssetImportOption& Option, std::filesystem::path& ImportedPath, Core::FString& Error) {
        Error.clear();
        ImportedPath.clear();

        const std::wstring Extension{Core::FilePath::ToLowerExtension(Source)};
        const bool Texture{Extension == L".png" || Extension == L".jpg" || Extension == L".jpeg" || Extension == L".bmp" || Extension == L".tga" || Extension == L".dds" || Extension == L".hdr"};

        if (!Texture && !Asset::IMeshImporter::IsSupported(Source)) {
            Error = "Unsupported import file type: " + Core::FilePath::ToUtf8(Source.filename());

            return false;
        }

        const auto Parent{Directory.empty() ? mContentRoot : ResolvePath(Directory, Error)};

        if (Parent.empty()) {
            return false;
        }

        std::error_code Code{};

        if (!std::filesystem::is_regular_file(Source, Code) || Code) {
            Error = "Cannot read the import source";

            return false;
        }

        std::filesystem::create_directories(Parent, Code);

        if (Code) {
            Error = "Cannot create the import destination: " + Code.message();

            return false;
        }

        const Core::FString Name{AssetFileName(Core::FilePath::ToUtf8(Source.stem()), "Asset")};
        const auto Base{Core::FilePath::FromUtf8(Name)};

        if (!Texture && !Option.mImportMesh && !Option.mImportSkeleton && Option.mImportAnimations) {
            auto Target{Parent / Base};

            Target += L".masset";

            if (!ImportMesh(Source, Target, Option, Error)) {
                return false;
            }

            ImportedPath = Target.lexically_relative(mContentRoot);

            return true;
        }

        auto Folder{Parent / Base};

        for (Core::Uint32 Index{}; !std::filesystem::create_directory(Folder, Code); ++Index) {
            if (Code && Code != std::errc::file_exists) {
                Error = "Cannot create the import folder: " + Code.message();

                return false;
            }

            Code.clear();

            Folder = Parent / (Base.wstring() + L"_" + std::to_wstring(Index + 1));
        }

        // 새로 만든 폴더라도 지운 에셋의 캐시가 같은 경로에 남아 있을 수 있으므로 먼저 은퇴시킵니다.
        RetireAssets(Folder);

        Asset::FAssetImportScope Scope{Folder};
        auto Target{Folder / Base};

        Target += L".masset";

        if (!(Texture ? ImportTexture(Source, Target, Error) : ImportMesh(Source, Target, Option, Error))) {
            RetireAssets(Folder);

            return false;
        }

        ImportedPath = Target.lexically_relative(mContentRoot);

        Scope.Commit();

        return true;
    }

    /**
     * @brief 원본 메시와 재질·외부 텍스처를 임포트 옵션에 따라 프로젝트 에셋으로 가져와 메시 참조를 저장합니다.
     */
    bool Asset::FAssetManager::ImportMesh(const std::filesystem::path& Source, std::filesystem::path& Target, const Asset::FAssetImportOption& Option, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Asset::AssetStats::MeshImport};
        const auto Destination{ResolvePath(Target, Error)};

        if (Destination.empty()) {
            return false;
        }

        Asset::FMeshImportResult Imported{};

        {
            auto Importer{Asset::IMeshImporter::CreateForSource(Source, Option)};

            if (!Importer->Import(Imported, Error)) {
                Error = "Cannot import mesh source '" + Core::FilePath::ToUtf8(Source) + "': " + Error;

                return false;
            }
        }

        // 메시 옵션을 끄면 원본에 메시가 있어도 스켈레톤과 애니메이션만 가져옵니다.
        if (!Option.mImportMesh) {
            Imported.GetMeshes().clear();
        }

        const bool HasSkeleton{Option.mImportSkeleton && !Imported.GetBones().empty()};
        std::filesystem::path SkeletonTarget{};

        if (HasSkeleton) {
            auto Skeleton{CoreUObject::NewObject<Asset::USkeleton>()};

            if (!Skeleton->SetBones(std::move(Imported.GetBones()))) {
                Error = "Invalid imported skeleton data";

                return false;
            }

            SkeletonTarget = UniqueAssetPath(Destination.parent_path(), Core::FilePath::ToUtf8(Destination.stem()) + "_Skeleton", Error);

            if (SkeletonTarget.empty() || !SaveSkeleton(SkeletonTarget, *Skeleton, Error)) {
                return false;
            }
        }

        std::filesystem::path FirstAnimationTarget{};

        if (Option.mImportAnimations) {
            const bool AnimationOnly{!Option.mImportMesh && !Option.mImportSkeleton};
            const auto AnimationDirectory{AnimationOnly ? Destination.parent_path() : Destination.parent_path() / "Animations"};
            Asset::FAssetImportScope AnimationScope{};

            for (Asset::FMeshImportAnimation& Animation : Imported.GetAnimations()) {
                auto Sequence{CoreUObject::NewObject<Asset::UAnimSequence>()};

                if (!Sequence->SetDataModel(std::move(Animation.GetDataModel()), Error)) {
                    Error = "Invalid imported animation '" + Animation.GetName() + "': " + Error;

                    return false;
                }

                const auto AnimationTarget{UniqueAssetPath(AnimationDirectory, AssetFileName(Animation.GetName(), "Animation"), Error)};

                if (AnimationTarget.empty()) {
                    return false;
                }

                if (AnimationOnly) {
                    RetireAssets(AnimationTarget);
                    AnimationScope.TrackFile(AnimationTarget);
                }

                if (!SaveAnimSequence(AnimationTarget, *Sequence, Error)) {
                    return false;
                }

                if (FirstAnimationTarget.empty()) {
                    FirstAnimationTarget = AnimationTarget;
                }
            }

            AnimationScope.Commit();
        }

        // 메시가 없으면 스켈레톤을, 스켈레톤도 없으면 첫 애니메이션을 대표 에셋으로 선택합니다.
        if (Imported.GetMeshes().empty()) {
            Target = !SkeletonTarget.empty() ? SkeletonTarget : FirstAnimationTarget;

            if (Target.empty()) {
                Error = "The source contains nothing to import with the selected options";

                return false;
            }

            return true;
        }

        const auto& Materials{Imported.GetMaterials()};
        const auto MaterialDirectory{Destination.parent_path() / "Materials"};
        const auto TextureDirectory{Destination.parent_path() / "Textures"};
        Core::TMap<Core::FString, Core::FString> Textures{};

        // 결과 재질별로 저장한 머티리얼 에셋 경로입니다. 여러 메시가 같은 재질을 쓰면 한 번만 저장하고, 쓰이지 않는 재질은 저장하지 않습니다.
        Core::TMap<Core::Uint32, Core::FString> MaterialPaths{};

        const auto ImportMaterial{[&](Core::Uint32 Index, Core::FString& MaterialPath) {
            const Asset::FMeshImportMaterial& SourceMaterial{Materials[Index]};
            auto Material{CoreUObject::NewObject<Asset::UMaterial>()};
            Material->SetName(SourceMaterial.GetName());

            if (!Material->SetBaseColor(SourceMaterial.GetBaseColor())) {
                Error = "Invalid imported material color: " + SourceMaterial.GetName();

                return false;
            }

            Material->SetTwoSided(SourceMaterial.IsTwoSided());

            if (!Material->SetSpecularColor(SourceMaterial.GetSpecularColor()) || !Material->SetSpecularStrength(SourceMaterial.GetSpecularStrength()) || !Material->SetShininess(SourceMaterial.GetShininess()) || !Material->SetAmbientColor(SourceMaterial.GetAmbientColor()) || !Material->SetEmissiveColor(SourceMaterial.GetEmissiveColor()) || !Material->SetReflectionColor(SourceMaterial.GetReflectionColor()) || !Material->SetReflectionStrength(SourceMaterial.GetReflectionStrength())) {
                Error = "Invalid imported material lighting: " + SourceMaterial.GetName();

                return false;
            }

            Core::FString BaseColorPath{};
            Core::FString NormalMapPath{};
            Core::FString SpecularPath{};
            Core::FString EmissivePath{};
            Core::FString ReflectionPath{};

            // 텍스처 임포트 옵션을 끄면 텍스처 경로를 비워 둔 채 머티리얼의 색상·조명 값만 저장합니다.
            const auto ReadTexture{[&](const Asset::FMeshImportTexture& Texture, Core::FString& TexturePath) {
                return !Option.mImportTextures || ImportMeshTexture(*this, Texture, TextureDirectory, Textures, TexturePath, Error);
            }};

            if (!ReadTexture(SourceMaterial.GetBaseColorTexture(), BaseColorPath) || !ReadTexture(SourceMaterial.GetNormalMapTexture(), NormalMapPath)) {
                return false;
            }

            if (!Material->SetBaseColorTexturePath(BaseColorPath) || !Material->SetNormalMapTexturePath(NormalMapPath)) {
                Error = "Invalid imported material texture path: " + SourceMaterial.GetName();

                return false;
            }

            if (!ReadTexture(SourceMaterial.GetSpecularTexture(), SpecularPath) || !ReadTexture(SourceMaterial.GetEmissiveTexture(), EmissivePath) || !ReadTexture(SourceMaterial.GetReflectionTexture(), ReflectionPath)) {
                return false;
            }

            if (!Material->SetSpecularTexturePath(SpecularPath) || !Material->SetEmissiveTexturePath(EmissivePath) || !Material->SetReflectionTexturePath(ReflectionPath)) {
                Error = "Invalid imported material texture path: " + SourceMaterial.GetName();

                return false;
            }

            const auto MaterialTarget{UniqueAssetPath(MaterialDirectory, AssetFileName(Material->GetName(), "Material_" + std::to_string(Index)), Error)};

            if (MaterialTarget.empty() || !SaveMaterial(MaterialTarget, *Material, Error)) {
                return false;
            }

            MaterialPath = GetRelativePath(MaterialTarget, Error);

            return !MaterialPath.empty();
        }};

        auto& Meshes{Imported.GetMeshes()};

        for (std::size_t MeshIndex{}; MeshIndex < Meshes.size(); ++MeshIndex) {
            Asset::FMeshImportMesh& ImportedMesh{Meshes[MeshIndex]};
            Asset::FMeshData& Data{ImportedMesh.GetMeshData()};
            const auto& MaterialIndices{ImportedMesh.GetMaterialIndices()};

            if (MaterialIndices.size() != Data.GetMaterialSlots().size()) {
                Error = "Imported materials do not match the mesh material slots";

                return false;
            }

            for (std::size_t Slot{}; Slot < MaterialIndices.size(); ++Slot) {
                const Core::Uint32 MaterialIndex{MaterialIndices[Slot]};

                if (MaterialIndex >= Materials.size()) {
                    Error = "An imported mesh slot references an invalid material";

                    return false;
                }

                // 재질 임포트 옵션을 끄면 슬롯의 머티리얼 경로를 비워 기본 머티리얼을 사용하게 합니다.
                Core::FString MaterialPath{};

                if (Option.mImportMaterials) {
                    auto Existing{MaterialPaths.find(MaterialIndex)};

                    if (Existing == MaterialPaths.end()) {
                        if (!ImportMaterial(MaterialIndex, MaterialPath)) {
                            return false;
                        }

                        Existing = MaterialPaths.emplace(MaterialIndex, std::move(MaterialPath)).first;
                    }

                    MaterialPath = Existing->second;
                }

                Data.GetMaterialSlots()[Slot] = Asset::FMeshMaterialSlot{Materials[MaterialIndex].GetName(), std::move(MaterialPath)};
            }

            // 메시가 하나이면 요청한 경로에 저장하고, 여러 개이면 같은 폴더에 메시 이름으로 저장합니다. 첫 메시가 대표 경로가 됩니다.
            const auto MeshTarget{Meshes.size() == 1 ? Destination : UniqueAssetPath(Destination.parent_path(), AssetFileName(ImportedMesh.GetName(), "Mesh_" + std::to_string(MeshIndex)), Error)};

            if (MeshTarget.empty()) {
                return false;
            }

            if (HasSkeleton) {
                auto Mesh{CoreUObject::NewObject<Asset::USkeletalMesh>()};
                Core::TArray<Asset::FMeshImportSkinWeight> SkinWeights{ImportedMesh.GetSkinWeights()};

                if (SkinWeights.empty()) {
                    SkinWeights.resize(Data.GetPositions().size());
                }

                if (!Mesh->SetMeshData(std::move(Data), std::move(SkinWeights))) {
                    Error = "Invalid imported skeletal mesh data: " + ImportedMesh.GetName();

                    return false;
                }

                if (!SaveSkeletalMesh(MeshTarget, *Mesh, Error)) {
                    return false;
                }
            } else {
                auto Mesh{CoreUObject::NewObject<Asset::UStaticMesh>()};

                if (!Mesh->SetMeshData(std::move(Data))) {
                    Error = "Invalid imported mesh sections: " + ImportedMesh.GetName();

                    return false;
                }

                if (!SaveMesh(MeshTarget, *Mesh, Error)) {
                    return false;
                }
            }

            if (MeshIndex == 0) {
                Target = MeshTarget;
            }
        }

        return true;
    }

    /**
     * @brief 외부 이미지를 읽어 프로젝트의 텍스처 에셋 파일로 저장합니다.
     */
    bool Asset::FAssetManager::ImportTexture(const std::filesystem::path& Source, const std::filesystem::path& Target, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Asset::AssetStats::TextureImport};
        auto Texture{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (!Texture->LoadFromFile(Source)) {
            Error = "Cannot import texture source";

            return false;
        }

        return SaveTexture(Target, *Texture, Error);
    }

    void Asset::FAssetManager::RetireAssets(const std::filesystem::path& Path) {
        Core::FString Error{};
        const Core::FString Relative{GetRelativePath(Path, Error)};

        if (Relative.empty()) {
            return;
        }

        // 콘텐츠 루트 자체를 가리키면 모든 항목이 대상입니다.
        const Core::FString Key{Relative == "." ? Core::FString{} : CacheKey(Relative)};
        const auto Retire{[this, &Key](auto& Cache) {
            std::erase_if(Cache, [this, &Key](auto& Entry) {
                if (!Key.empty() && Entry.first != Key && !Entry.first.starts_with(Key + "/")) {
                    return false;
                }

                mRetired.push_back(std::move(Entry.second));

                return true;
            });
        }};

        Retire(mMeshes);
        Retire(mSkeletalMeshes);
        Retire(mSkeletons);
        Retire(mLegacySkeletons);
        Retire(mAnimSequences);
        Retire(mTextures);
        Retire(mNormalMaps);
        Retire(mMaterials);
        Retire(mSequences);
        Retire(mBlendSpaces);
        Retire(mAnimGraphs);
    }

    /**
     * @brief 재질 에셋과 텍스처 참조를 로드하여 캐시에 보관합니다.
     */
    Asset::UMaterial* Asset::FAssetManager::LoadMaterial(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Asset::AssetStats::MaterialLoad};
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mMaterials.find(Key)};

        if (Existing != mMaterials.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        auto Material{CoreUObject::NewObject<Asset::UMaterial>()};

        if (!Asset::FMaterialFile::Load(ResolvePath(Path, Error), *this, *Material, Error)) {
            return nullptr;
        }

        Asset::UMaterial* Result{Material.get()};

        mMaterials.emplace(Key, std::move(Material));

        return Result;
    }

    /**
     * @brief 시퀀스 에셋을 로드하여 캐시에 보관합니다.
     */
    Asset::ULevelSequence* Asset::FAssetManager::LoadSequence(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mSequences.find(Key)};

        if (Existing != mSequences.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(ResolvePath(Path, Error), Asset::EAssetType::Sequence, Payload, Error)) {
            return nullptr;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};

        if (!Archive.Parse(Core::FString{reinterpret_cast<const char*>(Payload.data()), Payload.size()})) {
            Error = Archive.GetError();

            return nullptr;
        }

        auto Sequence{CoreUObject::NewObject<Asset::ULevelSequence>()};
        Sequence->Serialize(Archive);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return nullptr;
        }

        Asset::ULevelSequence* Result{Sequence.get()};

        mSequences.emplace(Key, std::move(Sequence));

        return Result;
    }

    /**
     * @brief Sequences 폴더에 빈 시퀀스 에셋을 만들고 캐시에 보관합니다.
     */
    Asset::ULevelSequence* Asset::FAssetManager::CreateSequence(const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error) {
        const auto Directory{mContentRoot / "Sequences"};
        std::error_code Code{};

        std::filesystem::create_directories(Directory, Code);

        if (Code) {
            Error = "Cannot create the sequence folder: " + Code.message();

            return nullptr;
        }

        const auto Target{UniqueAssetPath(Directory, AssetFileName(Name, "Sequence"), Error)};
        auto Sequence{CoreUObject::NewObject<Asset::ULevelSequence>()};

        if (Target.empty() || !SaveSequence(Target, *Sequence, Error)) {
            return nullptr;
        }

        Path = Target.lexically_relative(mContentRoot);

        Asset::ULevelSequence* Result{Sequence.get()};

        mSequences.emplace(CacheKey(GetRelativePath(Target, Error)), std::move(Sequence));

        return Result;
    }

    /**
     * @brief 시퀀스를 에셋 파일로 저장합니다.
     */
    bool Asset::FAssetManager::SaveSequence(const std::filesystem::path& Path, Asset::ULevelSequence& Sequence, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty()) {
            return false;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};

        Sequence.Serialize(Archive);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        const Core::FString Payload{Archive.ToString()};

        return Asset::FAssetFile::Save(Target, Asset::EAssetType::Sequence, std::span<const Core::Uint8>{reinterpret_cast<const Core::Uint8*>(Payload.data()), Payload.size()}, Error);
    }

    Asset::UBlendSpace* Asset::FAssetManager::LoadBlendSpace(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mBlendSpaces.find(Key)};

        if (Existing != mBlendSpaces.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(ResolvePath(Path, Error), Asset::EAssetType::BlendSpace, Payload, Error)) {
            return nullptr;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};

        if (!Archive.Parse(Core::FString{reinterpret_cast<const char*>(Payload.data()), Payload.size()})) {
            Error = Archive.GetError();

            return nullptr;
        }

        // 파일에 기록한 클래스 이름으로 1D·2D 같은 파생 타입을 고릅니다. 지금은 1D만 있습니다.
        Core::FString ClassName{};

        Archive.Serialize("Class", ClassName);

        if (Archive.HasError() || ClassName != "BlendSpace1D") {
            Error = Archive.HasError() ? Archive.GetError() : "Unsupported blend space class: " + ClassName;

            return nullptr;
        }

        std::unique_ptr<Asset::UBlendSpace> BlendSpace{CoreUObject::NewObject<Asset::UBlendSpace1D>()};
        BlendSpace->SetName(Core::FilePath::ToUtf8(Path.stem()));

        BlendSpace->Serialize(Archive, *this);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return nullptr;
        }

        Asset::UBlendSpace* Result{BlendSpace.get()};

        mBlendSpaces.emplace(Key, std::move(BlendSpace));

        return Result;
    }

    Asset::UBlendSpace* Asset::FAssetManager::CreateBlendSpace1D(const std::filesystem::path& Directory, const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error) {
        // 콘텐츠 루트 자체는 ResolvePath가 에셋 경로로 받지 않으므로 직접 씁니다.
        const auto Folder{Directory.empty() || Directory == L"." ? mContentRoot : ResolvePath(Directory, Error)};

        if (Folder.empty()) {
            return nullptr;
        }

        const auto Target{UniqueAssetPath(Folder, AssetFileName(Name, "BlendSpace"), Error)};

        if (Target.empty()) {
            return nullptr;
        }

        std::unique_ptr<Asset::UBlendSpace> BlendSpace{CoreUObject::NewObject<Asset::UBlendSpace1D>()};

        BlendSpace->SetName(Core::FilePath::ToUtf8(Target.stem()));

        if (!SaveBlendSpace(Target, *BlendSpace, Error)) {
            return nullptr;
        }

        const Core::FString Relative{GetRelativePath(Target, Error)};

        if (Relative.empty()) {
            return nullptr;
        }

        Path = Core::FilePath::FromUtf8(Relative);

        Asset::UBlendSpace* Result{BlendSpace.get()};

        mBlendSpaces.emplace(CacheKey(Relative), std::move(BlendSpace));

        return Result;
    }

    bool Asset::FAssetManager::SaveBlendSpace(const std::filesystem::path& Path, Asset::UBlendSpace& BlendSpace, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty()) {
            return false;
        }

        const Core::FString Key{CacheKey(GetRelativePath(Target, Error))};

        if (Key.empty()) {
            return false;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};
        Core::FString ClassName{"BlendSpace1D"};

        Archive.Serialize("Class", ClassName);
        BlendSpace.Serialize(Archive, *this);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        const Core::FString Payload{Archive.ToString()};

        if (!Asset::FAssetFile::Save(Target, Asset::EAssetType::BlendSpace, std::span<const Core::Uint8>{reinterpret_cast<const Core::Uint8*>(Payload.data()), Payload.size()}, Error)) {
            return false;
        }

        if (const auto Existing{mBlendSpaces.find(Key)}; Existing != mBlendSpaces.end()) {
            Existing->second->CopyFrom(BlendSpace);
        }

        return true;
    }

    Asset::UAnimGraph* Asset::FAssetManager::LoadAnimGraph(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mAnimGraphs.find(Key)};

        if (Existing != mAnimGraphs.end()) {
            Asset::AssetStats::AssetCacheHits.Add();

            return Existing->second.get();
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(ResolvePath(Path, Error), Asset::EAssetType::AnimGraph, Payload, Error)) {
            return nullptr;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};

        if (!Archive.Parse(Core::FString{reinterpret_cast<const char*>(Payload.data()), Payload.size()})) {
            Error = Archive.GetError();

            return nullptr;
        }

        auto Graph{CoreUObject::NewObject<Asset::UAnimGraph>()};

        Graph->Serialize(Archive);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return nullptr;
        }

        Graph->SetName(Core::FilePath::ToUtf8(Path.stem()));

        Asset::UAnimGraph* Result{Graph.get()};

        mAnimGraphs.emplace(Key, std::move(Graph));

        return Result;
    }

    Asset::UAnimGraph* Asset::FAssetManager::CreateAnimGraph(const std::filesystem::path& Directory, const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error) {
        // 콘텐츠 루트 자체는 ResolvePath가 에셋 경로로 받지 않으므로 직접 씁니다.
        const auto Folder{Directory.empty() || Directory == L"." ? mContentRoot : ResolvePath(Directory, Error)};

        if (Folder.empty()) {
            return nullptr;
        }

        const auto Target{UniqueAssetPath(Folder, AssetFileName(Name, "AnimGraph"), Error)};

        if (Target.empty()) {
            return nullptr;
        }

        auto Graph{CoreUObject::NewObject<Asset::UAnimGraph>()};

        Graph->SetName(Core::FilePath::ToUtf8(Target.stem()));

        if (!SaveAnimGraph(Target, *Graph, Error)) {
            return nullptr;
        }

        const Core::FString Relative{GetRelativePath(Target, Error)};

        if (Relative.empty()) {
            return nullptr;
        }

        Path = Core::FilePath::FromUtf8(Relative);

        Asset::UAnimGraph* Result{Graph.get()};

        mAnimGraphs.emplace(CacheKey(Relative), std::move(Graph));

        return Result;
    }

    bool Asset::FAssetManager::SaveAnimGraph(const std::filesystem::path& Path, Asset::UAnimGraph& Graph, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty()) {
            return false;
        }

        const Core::FString Key{CacheKey(GetRelativePath(Target, Error))};

        if (Key.empty()) {
            return false;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};

        Graph.Serialize(Archive);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        const Core::FString Payload{Archive.ToString()};

        if (!Asset::FAssetFile::Save(Target, Asset::EAssetType::AnimGraph, std::span<const Core::Uint8>{reinterpret_cast<const Core::Uint8*>(Payload.data()), Payload.size()}, Error)) {
            return false;
        }

        if (const auto Existing{mAnimGraphs.find(Key)}; Existing != mAnimGraphs.end()) {
            Existing->second->CopyFrom(Graph);
        }

        return true;
    }

    /**
     * @brief 머티리얼을 안전하게 저장하고 같은 경로의 캐시 객체를 갱신합니다.
     */
    bool Asset::FAssetManager::SaveMaterial(const std::filesystem::path& Path, const Asset::UMaterial& Material, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};
        auto Candidate{CoreUObject::NewObject<Asset::UMaterial>()};

        if (Target.empty() || !ApplyMaterialSettings(*Candidate, Material, Error) || !Asset::FMaterialFile::Save(Target, *this, *Candidate, Error)) {
            return false;
        }

        const auto Existing{mMaterials.find(CacheKey(GetRelativePath(Path, Error)))};

        if (Existing != mMaterials.end() && Existing->second.get() != &Material) {
            return ApplyMaterialSettings(*Existing->second, Material, Error);
        }

        return true;
    }

    /**
     * @brief 캐시 객체의 주소를 유지하면서 파일의 최신 설정을 다시 적용합니다.
     */
    bool Asset::FAssetManager::ReloadMaterial(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return false;
        }

        const auto Existing{mMaterials.find(Key)};

        if (Existing == mMaterials.end()) {
            return LoadMaterial(Path, Error) != nullptr;
        }

        auto Candidate{CoreUObject::NewObject<Asset::UMaterial>()};

        if (!Asset::FMaterialFile::Load(ResolvePath(Path, Error), *this, *Candidate, Error)) {
            return false;
        }

        return ApplyMaterialSettings(*Existing->second, *Candidate, Error);
    }

    /**
     * @brief 캐시 객체의 주소를 유지하면서 머티리얼 설정과 텍스처 참조를 갱신합니다.
     */
    bool Asset::FAssetManager::ApplyMaterialSettings(Asset::UMaterial& Target, const Asset::UMaterial& Source, Core::FString& Error) {
        Target.SetName(Source.GetName());
        Target.SetShaderPath(Source.GetShaderPath());
        Target.SetBaseColor(Source.GetBaseColor());
        Target.SetBaseColorTexturePath(Source.GetBaseColorTexturePath());
        Target.SetNormalMapTexturePath(Source.GetNormalMapTexturePath());
        Target.SetSpecularStrength(Source.GetSpecularStrength());
        Target.SetShininess(Source.GetShininess());
        Target.SetNormalStrength(Source.GetNormalStrength());
        Target.SetTwoSided(Source.IsTwoSided());
        Target.SetSpecularColor(Source.GetSpecularColor());
        Target.SetAmbientColor(Source.GetAmbientColor());
        Target.SetEmissiveColor(Source.GetEmissiveColor());
        Target.SetReflectionColor(Source.GetReflectionColor());
        Target.SetReflectionStrength(Source.GetReflectionStrength());
        Target.SetSpecularTexturePath(Source.GetSpecularTexturePath());
        Target.SetEmissiveTexturePath(Source.GetEmissiveTexturePath());
        Target.SetReflectionTexturePath(Source.GetReflectionTexturePath());

        return Target.ResolveTexture(*this, Error);
    }

    /**
     * @brief 노멀 맵을 선형 색상으로 로드하고 전용 캐시에 보관합니다.
     */
    Asset::UTexture2D* Asset::FAssetManager::LoadNormalMap(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mNormalMaps.find(Key)};

        if (Existing != mNormalMaps.end()) {
            return Existing->second.get();
        }

        Asset::UTexture2D* Source{LoadTexture(Path, Error)};

        if (Source == nullptr) {
            return nullptr;
        }

        Asset::FTextureAssetData Data{*Source};
        auto Texture{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (!Data.ApplyTo(*Texture, Error) || !Texture->SetSRGB(false)) {
            Error = "Cannot create a linear normal map: " + Error;

            return nullptr;
        }

        Asset::UTexture2D* Result{Texture.get()};

        mNormalMaps.emplace(Key, std::move(Texture));

        return Result;
    }

}
