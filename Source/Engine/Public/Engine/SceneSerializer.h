/**
 * @file SceneSerializer.h
 * @brief 월드, 액터와 컴포넌트의 장면 파일 직렬화를 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Engine/EWorldType.h"

#include <filesystem>

namespace Asset {
    class FAssetManager;
}

namespace Core {
    class FArchive;
}

namespace Engine {

    class FWorldContext;

    /**
     * @brief 리플렉션 프로퍼티를 사용하여 월드와 액터·컴포넌트를 JSON으로 저장하고 복원합니다.
     * @ingroup Engine
     */
    class FSceneSerializer {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FSceneSerializer);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSceneSerializer);

        /**
         * @brief 월드 컨텍스트를 JSON 장면 파일로 저장합니다.
         * @param[in] Path 읽거나 쓸 파일 경로입니다.
         * @param[in,out] Context 저장할 월드 컨텍스트입니다.
         * @param[in,out] Error 실패 시 원인을 기록할 문자열입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        static bool Save(const std::filesystem::path& Path, Engine::FWorldContext* Context, Core::FString& Error);

        /**
         * @brief 장면 파일을 읽고 검증한 새 월드 컨텍스트를 반환합니다.
         * @param[in] Path 읽을 장면 파일 경로입니다.
         * @param[out] Error 실패 시 원인을 기록할 문자열입니다.
         * @param[in] Assets 로드한 월드의 에셋을 조회할 관리자의 비소유 주소이며 생략할 수 있습니다.
         * @param[in] Type 새 월드의 용도입니다. 게임 실행에는 Game을, 편집에는 기본값 Editor를 사용합니다.
         * @return 생성한 월드 컨텍스트의 단독 소유권이며, 실패하면 nullptr입니다.
         */
        static std::unique_ptr<Engine::FWorldContext> Load(const std::filesystem::path& Path, Core::FString& Error, Asset::FAssetManager* Assets = nullptr, Engine::EWorldType Type = Engine::EWorldType::Editor);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         * @param[in,out] Context 직렬화할 월드 컨텍스트입니다.
         */
        static void Serialize(Core::FArchive& Archive, Engine::FWorldContext* Context);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();
    };

}
