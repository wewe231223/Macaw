#include "Engine/Cascade/FParticleSystemAssetManager.h"
#include "Engine/Cascade/UParticleSystem.h"
#include "Engine/Cascade/UParticleEmitter.h"
#include "Engine/Cascade/UParticleLODLevel.h"
#include "Engine/Cascade/UParticleModuleRequired.h"
#include "Asset/FAssetManager.h"
#include "Asset/FAssetFile.h"
#include "Asset/AssetStats.h"
#include "Core/FilePath.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Serialization/JsonArchive.h"

#include <algorithm>
#include <span>
#include <utility>

using namespace Engine;
using namespace Core;

FParticleSystemAssetManager::FParticleSystemAssetManager(Asset::FAssetManager& Assets)
	: mAssets{Assets} {
}

FParticleSystemAssetManager::~FParticleSystemAssetManager() = default;

/**
 * @brief 파티클 JSON 본문을 UClass 기반 정의로 복원하여 경로 캐시에 보관합니다.
 * @param[in] Path Content 기준 상대 경로 또는 콘텐츠 내부의 절대 경로입니다.
 * @param[out] Error 경로·파일·JSON·클래스 복원 오류를 받을 문자열입니다.
 * @return 캐시가 소유하는 시스템의 비소유 주소이며 실패하면 nullptr입니다.
 */
Engine::UParticleSystem* FParticleSystemAssetManager::LoadParticleSystem(const std::filesystem::path& Path, Core::FString& Error) {
    Error.clear();

    const auto Target{mAssets.ResolvePath(Path, Error)};
    const Core::FString Key{Target.empty() ? Core::FString{} : MakeCacheKey(mAssets.GetRelativePath(Target, Error))};

    if (Key.empty()) {
        return nullptr;
    }

    if (const auto Existing{mParticleSystems.find(Key)}; Existing != mParticleSystems.end()) {
        Asset::AssetStats::AssetCacheHits.Add();

        return Existing->second.get();
    }

    Core::TArray<Core::Uint8> Payload{};

    if (!Asset::FAssetFile::Load(Target, Asset::EAssetType::ParticleSystem, Payload, Error)) {
        return nullptr;
    }

    Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};

    if (!Archive.Parse(Core::FString{reinterpret_cast<const char*>(Payload.data()), Payload.size()})) {
        Error = Archive.GetError();

        return nullptr;
    }

    auto System{CoreUObject::NewObject<Engine::UParticleSystem>()};

    System->Serialize(Archive);

    if (Archive.HasError()) {
        Error = Archive.GetError();

        return nullptr;
    }

    if (!ResolveMaterials(*System, Error)) {
        return nullptr;
    }

    System->SetName(Core::FilePath::ToUtf8(Target.stem()));

    Engine::UParticleSystem* Result{System.get()};

    mParticleSystems.emplace(Key, std::move(System));

    return Result;
}

/**
 * @brief 고유한 .masset 이름으로 빈 파티클 정의를 만들고 저장한 객체를 캐시에 등록합니다.
 * @param[in] Directory 새 에셋을 만들 콘텐츠 폴더이며 빈 경로나 점은 콘텐츠 루트입니다.
 * @param[in] Name 파일 이름의 기본값입니다.
 * @param[out] Path 생성한 에셋의 콘텐츠 상대 경로이며 실패하면 비어 있습니다.
 * @param[out] Error 경로 해석 또는 저장 오류를 받을 문자열입니다.
 * @return 캐시가 소유하는 시스템의 비소유 주소이며 실패하면 nullptr입니다.
 */
Engine::UParticleSystem* FParticleSystemAssetManager::CreateParticleSystem(const std::filesystem::path& Directory, const Core::FString& Name, std::filesystem::path& Path, Core::FString& Error) {
    Error.clear();
    Path.clear();

    const auto Target{mAssets.MakeUniqueAssetPath(Directory, Name.empty() ? Core::FString{"ParticleSystem"} : Name, Error)};

    if (Target.empty()) {
        return nullptr;
    }

    auto System{CoreUObject::NewObject<Engine::UParticleSystem>()};

    System->SetName(Core::FilePath::ToUtf8(Target.stem()));

    if (!SaveParticleSystem(Target, *System, Error)) {
        return nullptr;
    }

    const Core::FString Relative{mAssets.GetRelativePath(Target, Error)};

    if (Relative.empty()) {
        return nullptr;
    }

    Path = Core::FilePath::FromUtf8(Relative);

    Engine::UParticleSystem* Result{System.get()};

    mParticleSystems.emplace(MakeCacheKey(Relative), std::move(System));

    return Result;
}

/**
 * @brief 파티클 정의의 JSON 본문을 .masset에 저장하고 캐시 UObject의 주소를 유지해 설정을 반영합니다.
 * @param[in] Path 저장할 콘텐츠 내부 에셋의 경로입니다.
 * @param[in,out] System 설정을 직렬화할 파티클 시스템입니다.
 * @param[out] Error 직렬화·후보 복원·파일 저장 오류를 받을 문자열입니다.
 * @return 파일 저장과 캐시 반영에 성공하면 true이며 실패하면 false입니다.
 * @note 인스턴스 실행 상태는 저장하지 않으며 실패한 후보는 캐시에 반영하지 않습니다.
 */
bool FParticleSystemAssetManager::SaveParticleSystem(const std::filesystem::path& Path, Engine::UParticleSystem& System, Core::FString& Error) {
    Error.clear();

    const auto Target{mAssets.ResolvePath(Path, Error)};
    const Core::FString Key{Target.empty() ? Core::FString{} : MakeCacheKey(mAssets.GetRelativePath(Target, Error))};

    if (Key.empty()) {
        return false;
    }

    Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};

    if (!ResolveMaterials(System, Error)) {
        return false;
    }

    System.Serialize(Archive);

    if (Archive.HasError()) {
        Error = Archive.GetError();

        return false;
    }

    const Core::FString Payload{Archive.ToString()};
    Serialization::FJsonArchive Validation{Core::EArchiveMode::Loading};
    auto Candidate{CoreUObject::NewObject<Engine::UParticleSystem>()};

    if (Validation.Parse(Payload)) {
        Candidate->Serialize(Validation);
    }

    if (Validation.HasError()) {
        Error = Validation.GetError();

        return false;
    }

    if (!ResolveMaterials(*Candidate, Error)) {
        return false;
    }

    if (!Asset::FAssetFile::Save(Target, Asset::EAssetType::ParticleSystem, std::span<const Core::Uint8>{reinterpret_cast<const Core::Uint8*>(Payload.data()), Payload.size()}, Error)) {
        return false;
    }

    if (const auto Existing{mParticleSystems.find(Key)}; Existing != mParticleSystems.end() && Existing->second.get() != &System) {
        Existing->second->MoveEmittersFrom(*Candidate);
    }

    return true;
}

void FParticleSystemAssetManager::RetireAssets(const std::filesystem::path& Path) {
    FString Error{};
    const FString Relative{mAssets.GetRelativePath(Path, Error)};

    if (Relative.empty()) {
        return;
    }

    const FString Key{Relative == "." ? FString{} : MakeCacheKey(Relative)};

    std::erase_if(mParticleSystems, [this, &Key](auto& Entry) {
        if (!Key.empty() && Entry.first != Key && !Entry.first.starts_with(Key + "/")) {
            return false;
        }

        mRetired.push_back(std::move(Entry.second));

        return true;
    });
}

FString FParticleSystemAssetManager::MakeCacheKey(const FString& Relative) const {
    return FilePath::ToGenericUtf8(FilePath::ToLower(FilePath::FromUtf8(Relative)));
}

bool FParticleSystemAssetManager::ResolveMaterials(UParticleSystem& System, FString& Error) {
    for (const auto& Emitter : System.GetEmitters()) {
        for (const auto& LODLevel : Emitter->GetLODLevels()) {
            if (!LODLevel->GetRequiredModule()->ResolveMaterial(mAssets, Error)) {
                return false;
            }
        }
    }

    return true;
}
