/**
 * @file SceneSerializer.cpp
 * @brief 월드, 액터와 컴포넌트의 장면 파일 직렬화를 제공합니다.
 * @ingroup Engine
 */

#include "Engine/SceneSerializer.h"
#include "Engine/FWorldSerializer.h"
#include "Engine/FWorldContext.h"
#include "Serialization/JsonArchive.h"

/**
 * @brief 월드 컨텍스트를 JSON 장면 파일로 저장합니다.
 */
bool FSceneSerializer::Save(const std::filesystem::path& Path, FWorldContext* Context, FString& Error) {
    FJsonArchive Archive{EArchiveMode::Saving};
    Serialize(Archive, Context);

    const bool Saved{!Archive.HasError() && Archive.Save(Path)};
    Error = Archive.GetError();

    return Saved;
}

/**
 * @brief 파일을 읽고 검증한 새 월드 컨텍스트를 반환합니다.
 */
std::unique_ptr<FWorldContext> FSceneSerializer::Load(const std::filesystem::path& Path, FString& Error) {
    FJsonArchive Archive{EArchiveMode::Loading};

    if (!Archive.Load(Path)) {
        Error = Archive.GetError();

        return nullptr;
    }

    auto Context{std::make_unique<FWorldContext>()};
    Serialize(Archive, Context.get());
    Error = Archive.GetError();

    return Archive.HasError() ? nullptr : std::move(Context);
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FSceneSerializer::Serialize(FArchive& Archive, FWorldContext* Context) {
    FWorldSerializer::Serialize(Archive, Context);
}
