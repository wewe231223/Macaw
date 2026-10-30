/**
 * @file SceneSerializer.cpp
 * @brief 월드, 액터와 컴포넌트의 장면 파일 직렬화를 제공합니다.
 * @ingroup Engine
 */

#include "Engine/SceneSerializer.h"
#include "Engine/FWorldSerializer.h"
#include "Engine/FWorldContext.h"
#include "Engine/FAssetFile.h"
#include "Engine/UWorld.h"
#include "Serialization/JsonArchive.h"

namespace Engine {

    /**
     * @brief 월드 컨텍스트를 JSON 장면 파일로 저장합니다.
     */
    bool Engine::FSceneSerializer::Save(const std::filesystem::path& Path, Engine::FWorldContext* Context, Core::FString& Error) {
        Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};
        Serialize(Archive, Context);

        if (Archive.HasError()) {
            Error = Archive.GetError();
            return false;
        }

        const Core::FString Payload{Archive.ToString()};

        return Engine::FAssetFile::Save(Path, Engine::EAssetType::Scene, std::span<const Core::Uint8>{reinterpret_cast<const Core::Uint8*>(Payload.data()), Payload.size()}, Error);
    }

    /**
     * @brief 파일을 읽고 검증한 새 월드 컨텍스트를 반환합니다.
     */
    std::unique_ptr<Engine::FWorldContext> Engine::FSceneSerializer::Load(const std::filesystem::path& Path, Core::FString& Error, Engine::FAssetManager* Assets) {
        Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};

        Core::TArray<Core::Uint8> Payload{};

        if (!Engine::FAssetFile::Load(Path, Engine::EAssetType::Scene, Payload, Error)) {
            return nullptr;
        }

        if (!Archive.Parse(Core::FString{reinterpret_cast<const char*>(Payload.data()), Payload.size()})) {
            Error = Archive.GetError();

            return nullptr;
        }

        auto Context{std::make_unique<Engine::FWorldContext>()};
        Context->GetWorld()->SetAssets(Assets);
        Serialize(Archive, Context.get());
        Error = Archive.GetError();

        return Archive.HasError() ? nullptr : std::move(Context);
    }

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
    void Engine::FSceneSerializer::Serialize(Core::FArchive& Archive, Engine::FWorldContext* Context) {
        Engine::FWorldSerializer::Serialize(Archive, Context);

        if (Archive.HasError() || Context == nullptr || (Archive.IsLoading() && !Archive.HasField("PerspectiveCamera"))) {
            return;
        }

        if (Archive.BeginObject("PerspectiveCamera")) {
            Context->GetEditorCamera().Serialize(Archive);
            Archive.EndObject();
        }
    }

}
